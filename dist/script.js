const TAURI = window.__TAURI__;
const MAX_LINES = 22;

TAURI.invoke('sideband_func')

let screen = document.getElementById('screen');

(async function listenAndHandle() {
    let now = function() {
        let today = new Date();
        let year = '' + today.getFullYear();
        let month = '' + (today.getMonth() + 1);
        let day = '' + today.getDate();
        let hours = '' + today.getHours();
        let minutes = '' + today.getMinutes();
        let seconds = '' + today.getSeconds();

        if (month.length < 2) month = '0' + month;
        if (day.length < 2) day = '0' + day;
        if (hours.length < 2) hours = '0' + hours;
        if (minutes.length < 2) minutes = '0' + minutes;
        if (seconds.length < 2) seconds = '0' + seconds;
        return [month, day, year].join('/') + ' ' + [hours, minutes, seconds].join(':');
    };

    let screenLines = 0;
    await TAURI.event.listen('event', e => {
        if (screen) {
            screen.innerHTML += JSON.stringify(e.payload) + '<br>';
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

let args = {
    repeat: -1 // negative value is run infinitely
};

// TAURI.invoke('start_trigger', args)
//     .then((resp) => {
//         if (resp == args.repeat) {
//             alert('Trigger is done.');
//         } else {
//             console.log(resp);
//         }
//     }).catch((err) => {
//         if (screen) {
//             screen.innerText = err;
//         }
//         alert(err);
//     });