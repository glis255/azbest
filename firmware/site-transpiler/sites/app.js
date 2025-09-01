const apiUrl = '';
const timeElement = document.getElementById('time');
const alarmsListElement = document.getElementById('alarms-list');
const planSelectElement = document.getElementById('plan-select');
const planName = document.getElementById('planName');
const saveButton = document.getElementById('save-button');
const resetButton = document.getElementById('reset-button');
const alarmNewText = document.getElementById('alarm-new');
const weekdaysElements = [
    document.getElementById('weekday-btn-pn'),
    document.getElementById('weekday-btn-wt'),
    document.getElementById('weekday-btn-sr'),
    document.getElementById('weekday-btn-cz'),
    document.getElementById('weekday-btn-pt'),
    document.getElementById('weekday-btn-so'),
    document.getElementById('weekday-btn-ni'),
];
const loginUsername = document.getElementById('username');
const loginPassword = document.getElementById('password');
const loginPasswordConfirm = document.getElementById('password-confirm');
const changeLoginButton = document.getElementById('change-password');
const loginError = document.getElementById('login-error');
const loginSuccess = document.getElementById('login-success');
const loadingScreenElement = document.getElementById('loading');
const wrapperElement = document.getElementById('wrapper');
const loadingText = document.getElementById('load-text');
const unsavedAlarmsElement = document.getElementById('unsaved-alarms');

const blankAlarm = '<span class="blank alarm-btn"></span><span class="alarm-text"></span><span class="btn-remove alarm-btn">✖</span>';

const loadPromises = [];

const api = (path) => {
    if (!apiUrl) return path;
    return apiUrl + '/' + path;
};

const minutesToTime = (minutes) => {
    const hours = parseInt(minutes / 60).toString();
    const minutess = parseInt(minutes % 60).toString();

    return `${hours.padStart(2, '0')}:${minutess.padStart(2, '0')}`;
};

let selectedPlan = 0;

let alarms = [];
let weekdays = 0;

const redrawWeekdays = () => {
    weekdaysElements.forEach((element, index) => {
        const mask = 1 << index;

        if ((weekdays & mask) != 0) {
            element.classList.add('weekday-active');
        } else {
            element.classList.remove('weekday-active');
        }
    });
};

const addNewAlarm = () => {
    unsavedAlarmsElement.classList.remove('display-none');

    const [hours, minutes] = alarmNewText.value.split(':').map(Number);

    if (isNaN(hours) || isNaN(minutes)) return;
    if (hours > 23 || hours < 0) return;
    if (minutes > 59 || minutes < 0) return;

    const alarmMinutes = hours * 60 + minutes;
    if (alarms.includes(alarmMinutes)) return;

    alarms.push(alarmMinutes);
    alarms.sort((a, b) => a - b);
    alarmNewText.value = '';
    constructList();
};

const getAlarms = async (plan) => {
    const res = await fetch(api(`alarms/${plan ?? 0}`));
    const data = await res.json();
    data.alarms.reverse();
    planName.value = data.name;
    weekdays = data.weekdays;
    alarms = data.alarms;
};

const constructList = () => {
    alarmsListElement.innerHTML = '';

    if (alarms.length) {
        alarms.forEach((alarm, index) => {
            const element = document.createElement('li');
            element.innerHTML = blankAlarm;

            const [input] = element.getElementsByClassName('alarm-text');
            input.innerText = minutesToTime(alarm);

            const buttons = element.getElementsByClassName('alarm-btn');
            const [_, removeButton] = buttons;

            removeButton.addEventListener('click', () => {
                alarms.splice(index, 1);
                constructList();
                unsavedAlarmsElement.classList.remove('display-none');
            });

            alarmsListElement.appendChild(element);
        });
    }
};

loadPromises.push(getAlarms(planSelectElement.value).then(a => {
    constructList();
    redrawWeekdays();
}));

loadPromises.push(fetch(api('clock')).then(res => {
    res.json().then(data => {
        let inc = 0;
        setInterval(() => {
            const date = new Date((data.epoch + inc) * 1000);
            timeElement.innerText = `${date.getHours().toString().padStart(2, '0')}:${date.getMinutes().toString().padStart(2, '0')}:${date.getSeconds().toString().padStart(2, '0')}`;
            inc++;
        }, 1000);

        document.getElementById("timezone").innerText = data.timezone;
        document.getElementById("mac-address").innerText = data.mac;
    });
}));

planSelectElement.addEventListener('change', () => {
    unsavedAlarmsElement.classList.add('display-none');
    getAlarms(planSelectElement.value).then(() => {
        constructList();
        redrawWeekdays();
    });
});

resetButton.addEventListener('click', () => {
    unsavedAlarmsElement.classList.add('display-none');
    getAlarms(planSelectElement.value).then(() => {
        constructList();
    });
});

alarmNewText.addEventListener('keydown', (e) => {
    if (e.key == 'Enter') {
        addNewAlarm();
    }
});

weekdaysElements.forEach((element, index) => {
    element.addEventListener('click', () => {
        unsavedAlarmsElement.classList.remove('display-none');
        weekdays ^= 1 << index;
        redrawWeekdays();
    });
});

saveButton.addEventListener('click', () => {
    unsavedAlarmsElement.classList.add('display-none');
    fetch(api(`alarms/${planSelectElement.value}`), {
        body: JSON.stringify({
            alarms: alarms.reverse(),
            weekdays,
            name: planName.value,
        }),
        method: 'POST',
    });
});

changeLoginButton.addEventListener('click', () => {
    loginError.innerText = "";
    loginSuccess.innerText = "";
    const usernameVal = loginUsername.value;
    const passwordVal = loginPassword.value;
    const passwordConfirmVal = loginPasswordConfirm.value;

    if (!passwordVal || !passwordConfirmVal || !usernameVal) {
        loginError.innerText = "Nie wypełniono pól";
        return;
    }
    
    if (passwordVal !== passwordConfirmVal) {
        loginError.innerText = "Hasła nie są takie same";
        return;
    }

    if (usernameVal.length > 25 || passwordVal.length > 25) {
        loginError.innerText = "Login lub hasło za długie. maksymalnie 25 znaków";
        return;
    }

    const base64auth = btoa(`${usernameVal}:${passwordVal}`);

    fetch(api('password'), {
        method: 'POST',
        body: JSON.stringify({password: base64auth}),
    }).then((res) => {
        if (res.ok) {
            loginSuccess.innerText = "Hasło zostało zmienione. Strona za chwilę zostanie odświeżona.";
            setTimeout(() => {
                location.reload();
            }, 5000);
        } else {
            loginError.innerText = "Nie udało się zmienić hasła.";
        }
    })
    .catch(() => {
        loginError.innerText = "Nie udało się zmienić hasła.";
    });
});

Promise.all(loadPromises).then(() => {
    loadingScreenElement.classList.add('display-none');
    wrapperElement.classList.remove('display-none');
})
.catch((p) => {
    console.error(p);
    loadingText.innerText = "Ładowanie nie powiodło się.";
});