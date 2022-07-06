const TAURI = window.__TAURI__;
const MAX_LINES = 33;


let screen = document.getElementById('screen');

(async function listenAndHandle() {
    
    let screenLines = 0;
    await TAURI.event.listen('event', e => {
        if (screen) {
            screen.innerHTML += JSON.stringify(e.payload).replace(/"/g,"") + '<br>';
            screenLines++;
            // cleanup
            if (screenLines > MAX_LINES) {
                screen.innerHTML = '';
                screenLines = 0;
            }
        } else {
            console.log(e);
        }
    });
})();
TAURI.invoke('sideband_func')