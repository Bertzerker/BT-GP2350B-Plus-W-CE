// Local UI preview of the real page exported by portal_test (no device credentials).
const http = require('node:http');
const fs = require('node:fs');
const path = require('node:path');
const root = path.join(__dirname, '..');
const routes = {
  '/': ['text/html', '.test-build/portal-preview.html'],
  '/favicon.png': ['image/png', '.build/web-assets/favicon.png'],
  '/background.jpg': ['image/jpeg', '.build/web-assets/background.jpg']
};
http.createServer((req, res) => {
  res.setHeader('Cache-Control', 'no-store');
  if (req.url === '/inputs') {
    res.setHeader('Content-Type', 'application/json');
    return res.end('{"pressed":16}'); // GP06 held for visual QA.
  }
  if (req.url === '/wiring-test.js') {
    res.setHeader('Content-Type', 'application/javascript');
    return res.end(fs.readFileSync(path.join(root, 'btpad_pico/WiringTest.h'), 'utf8').split('R"JS(')[1].split(')JS"')[0]);
  }
  const asset = routes[req.url];
  if (!asset) { res.writeHead(404); return res.end(); }
  res.setHeader('Content-Type', asset[0]);
  res.end(fs.readFileSync(path.join(root, asset[1])));
}).listen(8765, '127.0.0.1', () => console.log('Preview at http://127.0.0.1:8765'));
