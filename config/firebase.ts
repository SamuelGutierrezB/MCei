import { initializeApp } from 'firebase/app';
import { getDatabase } from 'firebase/database';

// Configuración de Firebase obtenida de google-services.json
const firebaseConfig = {
  apiKey: "AIzaSyDOMnu6pN35zScOxTtMmneKKfvFr7iCBek",
  authDomain: "mcei-5810e.firebaseapp.com",
  databaseURL: "https://mcei-5810e-default-rtdb.firebaseio.com",
  projectId: "mcei-5810e",
  storageBucket: "mcei-5810e.firebasestorage.app",
  messagingSenderId: "998632489953",
  appId: "1:998632489953:android:8c54c3a1b73a4ea42b08a0"
};

// Inicializar Firebase
const app = initializeApp(firebaseConfig);

// Obtener referencia a la base de datos
export const database = getDatabase(app);
export default app;
