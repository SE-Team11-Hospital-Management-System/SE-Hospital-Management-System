import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';

// The built website goes to web/dist, which the C++ server (hms) serves on http://localhost:8080.
// During development ("npm run dev") API calls are forwarded to the running C++ server.
export default defineConfig({
  plugins: [react()],
  server: { port: 5173, proxy: { '/api': 'http://localhost:8080' } },
  build: { outDir: 'dist', emptyOutDir: true, chunkSizeWarningLimit: 1500 },
});
