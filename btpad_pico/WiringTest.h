#pragma once

// Same-origin external script keeps executable inline content out of the portal.
const char WIRING_TEST_JS[] = R"JS(
(() => {
  const status = document.getElementById('wiring-status');
  const pins = Array.from(document.querySelectorAll('[data-input]'));
  function display(mask) {
    pins.forEach(pin => {
      const active = Boolean(mask & (1 << Number(pin.dataset.input)));
      pin.classList.toggle('pressed', active);
      pin.querySelector('.pin-state').textContent = active ? 'Pressed' : 'Released';
    });
  }
  async function poll() {
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), 2000);
    let again = true;
    let delay = 100;
    try {
      const response = await fetch('/inputs', {cache: 'no-store', signal: controller.signal});
      if (response.status === 401 || response.status === 403) {
        display(0);
        status.textContent = 'Sign in again to continue testing. Reload this page.';
        again = false;
        return;
      }
      if (!response.ok) throw new Error('Connection failed');
      const data = await response.json();
      if (!Number.isInteger(data.pressed) || data.pressed < 0 || data.pressed >= (1 << 19)) {
        throw new Error('Invalid input state');
      }
      display(data.pressed);
      status.textContent = data.pressed ? 'Live — highlighted pins are pressed.' : 'Live — press a button or move the joystick.';
    } catch (_) {
      display(0);
      status.textContent = 'Connection lost — reconnect to BTPad-Setup. Retrying…';
      delay = 1000;
    } finally {
      clearTimeout(timeout);
      if (again) setTimeout(poll, delay);
    }
  }
  poll();
})();
)JS";
