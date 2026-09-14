const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const source = fs.readFileSync(require('node:path').join(__dirname, '../btpad_pico/WiringTest.h'), 'utf8').split('R"JS(')[1].split(')JS"')[0];
async function run() {
  const status = {textContent: ''};
  const pins = [0, 16, 18, 18].map(id => ({dataset: {input: String(id)}, active: false,
    label: {}, classList: {toggle(_, value) { pins.find(p => p.classList === this).active = value; }},
    querySelector() {return this.label;}}));
  let response = {ok: true, status: 200, json: async () => ({pressed: (1 << 18) | 1})};
  const timers = new Map();
  let nextId = 0;
  const context = {document: {getElementById: () => status, querySelectorAll: () => pins},
    AbortController, fetch: async () => {if (response instanceof Error) throw response; return response;},
    setTimeout: (callback, delay) => {timers.set(++nextId, {callback, delay}); return nextId;},
    clearTimeout: id => timers.delete(id)};
  const settle = () => new Promise(resolve => setImmediate(resolve));
  async function tick() {
    const [id, timer] = [...timers][0];
    timers.delete(id);
    timer.callback();
    await settle();
  }
  vm.runInNewContext(source, context);
  await settle();
  assert.deepEqual(pins.map(p => p.active), [true, false, true, true]);
  assert.equal(pins[3].label.textContent, 'Pressed');
  response = new Error('offline');
  await tick();
  assert(pins.every(p => !p.active));
  assert.match(status.textContent, /Connection lost/);
  assert.equal([...timers.values()][0].delay, 1000);
  response = {ok: true, status: 200, json: async () => ({pressed: 1 << 16})};
  await tick();
  assert.deepEqual(pins.map(p => p.active), [false, true, false, false]);
  response = {ok: false, status: 401};
  await tick();
  assert(pins.every(p => !p.active));
  assert.match(status.textContent, /Sign in again/);
  assert.equal(timers.size, 0);
  console.log('Wiring script tests passed (presses, duplicate badges, failure, recovery, expiry).');
}
run().catch(error => {console.error(error); process.exitCode = 1;});
