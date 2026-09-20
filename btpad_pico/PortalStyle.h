#pragma once
const char PORTAL_STYLE[] = R"CSS(
*{box-sizing:border-box}html{min-height:100%;background:#122b3d url('/background.jpg') center top/cover fixed}
body{font-family:system-ui,sans-serif;color:#183247;margin:0;padding:20px 12px;line-height:1.5}
main{max-width:760px;margin:auto;padding:24px;background:rgba(248,251,255,.96);border:1px solid #ffffff66;border-radius:18px;box-shadow:0 8px 40px #05162466}
h1{display:flex;align-items:center;gap:12px;font-size:1.5rem;margin:0 0 20px}h1 img{width:44px;height:44px;border-radius:50%}
h2{font-size:1.15rem;margin:24px 0 8px}p{margin:10px 0;color:#41596c}label{display:block;margin:12px 0;font-weight:600}
input,select,button{font:inherit;font-size:16px;max-width:100%;min-height:44px;border-radius:8px}
input:not([type=checkbox]):not([type=hidden]),select{display:block;width:100%;padding:9px;border:1px solid #91a7b7;background:white;color:#183247;margin-top:5px;min-width:0}
input[type=checkbox]{width:22px;height:22px;min-height:22px;vertical-align:middle;accent-color:#075ebb;margin:0 8px 0 0}
button{padding:10px 18px;border:0;background:#075ebb;color:white;font-weight:650;cursor:pointer}button:hover{background:#064b96}
:focus-visible{outline:3px solid #ec9a16;outline-offset:2px}form{margin:14px 0}form[action='/password'],form[action='/logout']{padding-top:8px}
table{width:100%;border-collapse:collapse;table-layout:fixed;margin:14px 0}th,td{padding:8px 5px;text-align:left;vertical-align:middle;overflow-wrap:anywhere}
th{font-size:.8rem;color:#41596c}th:last-child,td:last-child{width:62px;text-align:center}td{border-top:1px solid #d6e0e8}
td select{margin:0}td label{display:flex;align-items:center;justify-content:center;margin:0;min-height:44px}td input[type=checkbox]{margin:0}
.pin{display:block;padding:6px 8px;border:2px solid #bacbd7;border-radius:8px;background:#eaf0f5;color:#203f55;font-size:.88rem;font-weight:700}
.pin.pressed{background:#146839;color:white;border-color:#0d4c29}.pin-state{display:block;font-size:.72rem;font-weight:400}
#wiring-status{font-size:.85rem;min-height:1.5em}.hint{font-size:.85rem}.fixed{font-size:.82rem;color:#526878}
@media(max-width:480px){body{padding:8px}main{padding:14px 10px;border-radius:12px}h1{font-size:1.25rem;gap:9px}h1 img{width:36px;height:36px}th,td{padding:7px 3px}th:last-child,td:last-child{width:46px}.pin{padding:6px 4px;font-size:.78rem}td select{padding:8px 3px}button{width:100%}}
)CSS";
