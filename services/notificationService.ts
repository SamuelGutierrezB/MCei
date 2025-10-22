import * as Notifications from 'expo-notifications';
import { Platform } from 'react-native';

// Configurar cómo se manejan las notificaciones cuando la app está en primer plano
Notifications.setNotificationHandler({
  handleNotification: async () => ({
    shouldShowAlert: true,
    shouldPlaySound: true,
    shouldSetBadge: false,
    shouldShowBanner: true,
    shouldShowList: true,
  }),
});

export interface NotificationService {
  requestPermissions: () => Promise<boolean>;
  scheduleNotification: (title: string, body: string, data?: any) => Promise<string | undefined>;
  sendActuadorNotification: (actuador: string, estado: boolean) => Promise<void>;
}

class NotificationServiceImpl implements NotificationService {
  private permissionsGranted = false;

  async requestPermissions(): Promise<boolean> {
    if (Platform.OS === 'android') {
      await Notifications.setNotificationChannelAsync('actuadores', {
        name: 'Actuadores',
        importance: Notifications.AndroidImportance.HIGH,
        vibrationPattern: [0, 250, 250, 250],
        lightColor: '#007AFF',
      });
    }

    const { status: existingStatus } = await Notifications.getPermissionsAsync();
    let finalStatus = existingStatus;

    if (existingStatus !== 'granted') {
      const { status } = await Notifications.requestPermissionsAsync();
      finalStatus = status;
    }

    this.permissionsGranted = finalStatus === 'granted';

    if (!this.permissionsGranted) {
      console.warn('No se otorgaron permisos de notificación');
    }

    return this.permissionsGranted;
  }

  async scheduleNotification(
    title: string,
    body: string,
    data?: any
  ): Promise<string | undefined> {
    if (!this.permissionsGranted) {
      console.warn('No se pueden enviar notificaciones sin permisos');
      return;
    }

    try {
      const notificationId = await Notifications.scheduleNotificationAsync({
        content: {
          title,
          body,
          data,
          sound: true,
          priority: Notifications.AndroidNotificationPriority.HIGH,
        },
        trigger: null, // Enviar inmediatamente
      });

      return notificationId;
    } catch (error) {
      console.error('Error al programar notificación:', error);
    }
  }

  async sendActuadorNotification(actuador: string, estado: boolean): Promise<void> {
    const emojis: { [key: string]: string } = {
      bomba: '💧',
      ventilador: '🌬️',
      luz: '💡',
    };

    const emoji = emojis[actuador.toLowerCase()] || '⚙️';
    const estadoTexto = estado ? 'ACTIVADO' : 'DESACTIVADO';
    const title = `${emoji} ${actuador.charAt(0).toUpperCase() + actuador.slice(1)} ${estadoTexto}`;
    const body = `El ${actuador} ha sido ${estado ? 'activado' : 'desactivado'}`;

    await this.scheduleNotification(title, body, {
      actuador,
      estado,
      timestamp: new Date().toISOString(),
    });
  }
}

// Exportar una instancia singleton
export const notificationService = new NotificationServiceImpl();

// Hook para usar notificaciones en componentes
export function useNotifications() {
  const [permissionsGranted, setPermissionsGranted] = React.useState(false);

  React.useEffect(() => {
    notificationService.requestPermissions().then(setPermissionsGranted);
  }, []);

  return {
    permissionsGranted,
    sendActuadorNotification: notificationService.sendActuadorNotification.bind(notificationService),
    scheduleNotification: notificationService.scheduleNotification.bind(notificationService),
  };
}

// Importar React para el hook
import React from 'react';
