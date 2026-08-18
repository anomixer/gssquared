const CACHE_NAME = 'gssquared-cache-v1';
const ASSETS_TO_CACHE = [
  './GSSquared.html',
  './GSSquared.js',
  './GSSquared.wasm',
  './GSSquared.data',
  './manifest.json'
];

self.addEventListener('install', (event) => {
  event.waitUntil(
    caches.open(CACHE_NAME).then((cache) => {
      return cache.addAll(ASSETS_TO_CACHE);
    }).then(() => self.skipWaiting())
  );
});

self.addEventListener('activate', (event) => {
  event.waitUntil(
    caches.keys().then((keys) => {
      return Promise.all(
        keys.map((key) => {
          if (key !== CACHE_NAME) {
            return caches.delete(key);
          }
        })
      );
    }).then(() => self.clients.claim())
  );
});

self.addEventListener('fetch', (event) => {
  // Network-first strategy with cache fallback for offline execution
  event.respondWith(
    fetch(event.request).catch(() => caches.match(event.request))
  );
});
