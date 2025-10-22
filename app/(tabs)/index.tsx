import { ThemedText } from "@/components/themed-text";
import { ThemedView } from "@/components/themed-view";
import { database } from "@/config/firebase";
import { onValue, ref } from "firebase/database";
import { useEffect, useRef, useState } from "react";
import {
  ActivityIndicator,
  Animated,
  ScrollView,
  StyleSheet,
  View,
} from "react-native";

interface ESP32Data {
  sensores?: {
    temperatura?: number;
    humedad?: number;
    sensacion_termica?: number;
    presion?: number;
    altitud?: number;
    luz?: number;
  };
  actuadores?: {
    bomba?: boolean;
    ventilador?: boolean;
    luz?: boolean;
  };
  umbrales?: {
    humedad?: number;
    temperatura?: number;
    luz?: number;
  };
  timestamp?: string;
  timestamp_millis?: number;
}

interface UltimaLectura {
  temperatura?: number;
  humedad?: number;
  luz?: number;
  bomba_activa?: boolean;
  ventilador_activo?: boolean;
  luz_activa?: boolean;
}

export default function HomeScreen() {
  const [data, setData] = useState<ESP32Data | null>(null);
  const [ultimaLectura, setUltimaLectura] = useState<UltimaLectura | null>(
    null
  );
  const [loading, setLoading] = useState(true);
  const [connected, setConnected] = useState(false);

  // Estados para el efecto de parpadeo
  const [changedValues, setChangedValues] = useState<Set<string>>(new Set());
  const previousData = useRef<ESP32Data | null>(null);
  const blinkAnimation = useRef(new Animated.Value(1)).current;

  // Función para detectar cambios en los valores
  const detectChanges = (newData: ESP32Data) => {
    if (!previousData.current) {
      previousData.current = newData;
      return;
    }

    const changed = new Set<string>();
    const prev = previousData.current;

    // Comparar valores de sensores
    if (newData.sensores?.temperatura !== prev.sensores?.temperatura) {
      changed.add("temperatura");
    }
    if (newData.sensores?.humedad !== prev.sensores?.humedad) {
      changed.add("humedad");
    }
    if (newData.sensores?.luz !== prev.sensores?.luz) {
      changed.add("luz");
    }
    if (newData.sensores?.presion !== prev.sensores?.presion) {
      changed.add("presion");
    }
    if (
      newData.sensores?.sensacion_termica !== prev.sensores?.sensacion_termica
    ) {
      changed.add("sensacion_termica");
    }
    if (newData.sensores?.altitud !== prev.sensores?.altitud) {
      changed.add("altitud");
    }

    if (changed.size > 0) {
      setChangedValues(changed);

      // Iniciar animación de parpadeo
      blinkAnimation.setValue(1);
      Animated.sequence([
        Animated.timing(blinkAnimation, {
          toValue: 0.3,
          duration: 200,
          useNativeDriver: false,
        }),
        Animated.timing(blinkAnimation, {
          toValue: 1,
          duration: 200,
          useNativeDriver: false,
        }),
        Animated.timing(blinkAnimation, {
          toValue: 0.3,
          duration: 200,
          useNativeDriver: false,
        }),
        Animated.timing(blinkAnimation, {
          toValue: 1,
          duration: 200,
          useNativeDriver: false,
        }),
      ]).start(() => {
        // Limpiar los cambios después de la animación
        setTimeout(() => setChangedValues(new Set()), 500);
      });
    }

    previousData.current = newData;
  };

  useEffect(() => {
    // Referencia a los datos principales del invernadero
    const datosRef = ref(database, "invernadero/datos");
    const lecturaRef = ref(database, "invernadero/ultima_lectura");

    // Escuchar cambios en los datos principales
    const unsubscribeDatos = onValue(
      datosRef,
      (snapshot) => {
        setLoading(false);
        if (snapshot.exists()) {
          const newData = snapshot.val();
          detectChanges(newData);
          setData(newData);
          setConnected(true);
        } else {
          setConnected(false);
        }
      },
      (error) => {
        console.error("Error al obtener datos:", error);
        setLoading(false);
        setConnected(false);
      }
    );

    // Escuchar cambios en la última lectura
    const unsubscribeLectura = onValue(lecturaRef, (snapshot) => {
      if (snapshot.exists()) {
        setUltimaLectura(snapshot.val());
      }
    });

    // Limpiar las suscripciones cuando el componente se desmonte
    return () => {
      unsubscribeDatos();
      unsubscribeLectura();
    };
  }, []);

  if (loading) {
    return (
      <ThemedView style={styles.container}>
        <ActivityIndicator size="large" color="#007AFF" />
        <ThemedText style={styles.loadingText}>
          Conectando con Firebase...
        </ThemedText>
      </ThemedView>
    );
  }

  console.log("Datos recibidos:", data);

  return (
    <ScrollView style={styles.scrollView}>
      <ThemedView style={styles.container}>
        <ThemedView style={styles.header}>
          <ThemedText type="title" style={styles.title}>
            Dashboard ESP32
          </ThemedText>
          <View
            style={[
              styles.statusIndicator,
              connected ? styles.connected : styles.disconnected,
            ]}
          >
            <ThemedText style={styles.statusText}>
              {connected ? "● Conectado" : "● Desconectado"}
            </ThemedText>
          </View>
          {/* Timestamp */}
          {data.timestamp_millis && (
            <ThemedView style={styles.infoContainer}>
              <ThemedText style={styles.infoText}>
                Última actualización:{" "}
                {new Date(data.timestamp_millis).toLocaleString()}
              </ThemedText>
            </ThemedView>
          )}
        </ThemedView>

        {connected && data ? (
          <>
            {/* Sección de Sensores Ambientales */}
            <ThemedView style={styles.section}>
              <ThemedText type="subtitle" style={styles.sectionTitle}>
                Sensores Ambientales
              </ThemedText>
              <ThemedView style={styles.cardContainer}>
                <Animated.View
                  style={[
                    styles.card,
                    styles.cardTemperature,
                    changedValues.has("temperatura") && {
                      opacity: blinkAnimation,
                    },
                  ]}
                >
                  <ThemedText style={styles.cardIcon}>🌡️</ThemedText>
                  <ThemedText type="subtitle" style={styles.cardTitle}>
                    Temperatura
                  </ThemedText>
                  <ThemedText style={styles.cardValue}>
                    {data.sensores?.temperatura?.toFixed(1) ?? "--"}°C
                  </ThemedText>
                </Animated.View>

                <Animated.View
                  style={[
                    styles.card,
                    styles.cardHumidity,
                    changedValues.has("humedad") && { opacity: blinkAnimation },
                  ]}
                >
                  <ThemedText style={styles.cardIcon}>💧</ThemedText>
                  <ThemedText type="subtitle" style={styles.cardTitle}>
                    Humedad
                  </ThemedText>
                  <ThemedText style={styles.cardValue}>
                    {data.sensores?.humedad?.toFixed(1) ?? "--"}%
                  </ThemedText>
                </Animated.View>

                <Animated.View
                  style={[
                    styles.card,
                    styles.cardLight,
                    changedValues.has("luz") && { opacity: blinkAnimation },
                  ]}
                >
                  <ThemedText style={styles.cardIcon}>☀️</ThemedText>
                  <ThemedText type="subtitle" style={styles.cardTitle}>
                    Luz
                  </ThemedText>
                  <ThemedText style={styles.cardValue}>
                    {data.sensores?.luz?.toFixed(0) ?? "--"} lx
                  </ThemedText>
                </Animated.View>

                <Animated.View
                  style={[
                    styles.card,
                    styles.cardPressure,
                    changedValues.has("presion") && { opacity: blinkAnimation },
                  ]}
                >
                  <ThemedText style={styles.cardIcon}>🌀</ThemedText>
                  <ThemedText type="subtitle" style={styles.cardTitle}>
                    Presión
                  </ThemedText>
                  <ThemedText style={styles.cardValue}>
                    {data.sensores?.presion?.toFixed(1) ?? "--"} hPa
                  </ThemedText>
                </Animated.View>

                <Animated.View
                  style={[
                    styles.card,
                    styles.cardFeelsLike,
                    changedValues.has("sensacion_termica") && {
                      opacity: blinkAnimation,
                    },
                  ]}
                >
                  <ThemedText style={styles.cardIcon}>🌡️</ThemedText>
                  <ThemedText type="subtitle" style={styles.cardTitle}>
                    Sensación Térmica
                  </ThemedText>
                  <ThemedText style={styles.cardValue}>
                    {data.sensores?.sensacion_termica?.toFixed(1) ?? "--"}°C
                  </ThemedText>
                </Animated.View>

                <Animated.View
                  style={[
                    styles.card,
                    styles.cardAltitude,
                    changedValues.has("altitud") && { opacity: blinkAnimation },
                  ]}
                >
                  <ThemedText style={styles.cardIcon}>⛰️</ThemedText>
                  <ThemedText type="subtitle" style={styles.cardTitle}>
                    Altitud
                  </ThemedText>
                  <ThemedText style={styles.cardValue}>
                    {data.sensores?.altitud?.toFixed(0) ?? "--"} m
                  </ThemedText>
                </Animated.View>
              </ThemedView>
            </ThemedView>

            {/* Sección de Actuadores */}
            <ThemedView style={styles.section}>
              <ThemedText type="subtitle" style={styles.sectionTitle}>
                Estado de Actuadores
              </ThemedText>
              <ThemedView style={styles.actuadoresContainer}>
                <ThemedView
                  style={[
                    styles.actuadorCard,
                    data.actuadores?.bomba
                      ? styles.actuadorActive
                      : styles.actuadorInactive,
                  ]}
                >
                  <ThemedText style={styles.actuadorIcon}>💧</ThemedText>
                  <ThemedText style={styles.actuadorTitle}>Bomba</ThemedText>
                  <ThemedText style={styles.actuadorStatus}>
                    {data.actuadores?.bomba ? "ACTIVA" : "INACTIVA"}
                  </ThemedText>
                </ThemedView>

                <ThemedView
                  style={[
                    styles.actuadorCard,
                    data.actuadores?.ventilador
                      ? styles.actuadorActive
                      : styles.actuadorInactive,
                  ]}
                >
                  <ThemedText style={styles.actuadorIcon}>🌬️</ThemedText>
                  <ThemedText style={styles.actuadorTitle}>
                    Ventilador
                  </ThemedText>
                  <ThemedText style={styles.actuadorStatus}>
                    {data.actuadores?.ventilador ? "ACTIVO" : "INACTIVO"}
                  </ThemedText>
                </ThemedView>

                <ThemedView
                  style={[
                    styles.actuadorCard,
                    data.actuadores?.luz
                      ? styles.actuadorActive
                      : styles.actuadorInactive,
                  ]}
                >
                  <ThemedText style={styles.actuadorIcon}>💡</ThemedText>
                  <ThemedText style={styles.actuadorTitle}>Luz</ThemedText>
                  <ThemedText style={styles.actuadorStatus}>
                    {data.actuadores?.luz ? "ACTIVA" : "INACTIVA"}
                  </ThemedText>
                </ThemedView>
              </ThemedView>
            </ThemedView>

            {/* Sección de Umbrales */}
            {data.umbrales && (
              <ThemedView style={styles.section}>
                <ThemedText type="subtitle" style={styles.sectionTitle}>
                  Umbrales Configurados
                </ThemedText>
                <ThemedView style={styles.thresholdsContainer}>
                  <ThemedView style={styles.thresholdItem}>
                    <ThemedText style={styles.thresholdLabel}>
                      💧 Humedad:
                    </ThemedText>
                    <ThemedText style={styles.thresholdValue}>
                      {data.umbrales.humedad}%
                    </ThemedText>
                  </ThemedView>
                  <ThemedView style={styles.thresholdItem}>
                    <ThemedText style={styles.thresholdLabel}>
                      🌡️ Temperatura:
                    </ThemedText>
                    <ThemedText style={styles.thresholdValue}>
                      {data.umbrales.temperatura}°C
                    </ThemedText>
                  </ThemedView>
                  <ThemedView style={styles.thresholdItem}>
                    <ThemedText style={styles.thresholdLabel}>
                      ☀️ Luz:
                    </ThemedText>
                    <ThemedText style={styles.thresholdValue}>
                      {data.umbrales.luz} lx
                    </ThemedText>
                  </ThemedView>
                </ThemedView>
              </ThemedView>
            )}
          </>
        ) : (
          <ThemedView style={styles.noDataContainer}>
            <ThemedText style={styles.noDataText}>
              No hay datos disponibles. Asegúrate de que tu ESP32 esté enviando
              datos a Firebase.
            </ThemedText>
            <ThemedText style={styles.infoText}>
              Ruta esperada: invernadero/datos
            </ThemedText>
          </ThemedView>
        )}
      </ThemedView>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  scrollView: {
    flex: 1,
  },
  container: {
    flex: 1,
    padding: 20,
  },
  header: {
    marginBottom: 24,
    marginTop: 40,
    alignItems: "center",
  },
  title: {
    marginBottom: 10,
    fontSize: 32,
    fontWeight: "bold",
  },
  statusIndicator: {
    paddingHorizontal: 16,
    paddingVertical: 8,
    borderRadius: 20,
    marginTop: 10,
  },
  connected: {
    backgroundColor: "#4CAF50",
  },
  disconnected: {
    backgroundColor: "#F44336",
  },
  statusText: {
    color: "#FFFFFF",
    fontWeight: "600",
    fontSize: 14,
  },
  section: {
    marginBottom: 32,
  },
  sectionTitle: {
    fontSize: 20,
    fontWeight: "600",
    marginBottom: 16,
    paddingLeft: 4,
  },
  cardContainer: {
    flexDirection: "row",
    justifyContent: "space-around",
    flexWrap: "wrap",
    gap: 12,
  },
  card: {
    flex: 1,
    minWidth: 140,
    maxWidth: 180,
    padding: 16,
    borderRadius: 16,
    backgroundColor: "#f5f5f5",
    alignItems: "center",
    shadowColor: "#000",
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.1,
    shadowRadius: 4,
    elevation: 3,
  },
  cardIcon: {
    fontSize: 32,
    marginBottom: 8,
  },
  cardTitle: {
    fontSize: 12,
    marginBottom: 8,
    opacity: 0.7,
    textAlign: "center",
    color: "#333333",
  },
  cardValue: {
    fontSize: 24,
    fontWeight: "bold",
    color: "#007AFF",
  },
  cardTemperature: {
    borderLeftWidth: 4,
    borderLeftColor: "#FF6B6B",
  },
  cardHumidity: {
    borderLeftWidth: 4,
    borderLeftColor: "#4ECDC4",
  },
  cardLight: {
    borderLeftWidth: 4,
    borderLeftColor: "#FFD93D",
  },
  cardPressure: {
    borderLeftWidth: 4,
    borderLeftColor: "#95E1D3",
  },
  cardFeelsLike: {
    borderLeftWidth: 4,
    borderLeftColor: "#F38181",
  },
  cardAltitude: {
    borderLeftWidth: 4,
    borderLeftColor: "#AA96DA",
  },
  actuadoresContainer: {
    flexDirection: "row",
    justifyContent: "space-around",
    flexWrap: "wrap",
    gap: 16,
  },
  actuadorCard: {
    flex: 1,
    minWidth: 100,
    padding: 16,
    borderRadius: 12,
    alignItems: "center",
    shadowColor: "#000",
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.1,
    shadowRadius: 4,
    elevation: 3,
  },
  actuadorActive: {
    backgroundColor: "#4CAF50",
  },
  actuadorInactive: {
    backgroundColor: "#9E9E9E",
  },
  actuadorIcon: {
    fontSize: 32,
    marginBottom: 8,
  },
  actuadorTitle: {
    fontSize: 14,
    fontWeight: "600",
    marginBottom: 4,
    color: "#FFFFFF",
  },
  actuadorStatus: {
    fontSize: 12,
    color: "#FFFFFF",
    fontWeight: "bold",
  },
  thresholdsContainer: {
    backgroundColor: "#f8f9fa",
    padding: 16,
    borderRadius: 12,
    borderWidth: 1,
    borderColor: "#e9ecef",
  },
  thresholdItem: {
    flexDirection: "row",
    justifyContent: "space-between",
    alignItems: "center",
    paddingVertical: 10,
    borderBottomWidth: 1,
    borderBottomColor: "#dee2e6",
    backgroundColor: "#f8f9fa",
  },
  thresholdLabel: {
    fontSize: 14,
    fontWeight: "500",
    color: "#495057",
  },
  thresholdValue: {
    fontSize: 16,
    fontWeight: "bold",
    color: "#007AFF",
  },
  infoContainer: {
    padding: 16,
    borderRadius: 8,
    backgroundColor: "#e8f4fd",
    marginTop: 10,
    marginBottom: 20,
    borderWidth: 1,
    borderColor: "#bee5eb",
  },
  infoText: {
    fontSize: 14,
    textAlign: "center",
    color: "#0c5460",
    fontWeight: "500",
  },
  noDataContainer: {
    padding: 30,
    alignItems: "center",
  },
  noDataText: {
    fontSize: 16,
    textAlign: "center",
    color: "#6c757d",
    marginBottom: 10,
    lineHeight: 24,
  },
  loadingText: {
    marginTop: 16,
    fontSize: 16,
  },
});
