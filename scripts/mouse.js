const menu = document.getElementById('custom-menu');

document.addEventListener('contextmenu', (e) => {
    e.preventDefault(); // Kill the browser's default menu

    menu.hidden = false;

    let x = e.clientX;
    let y = e.clientY;

    const menuWidth = menu.offsetWidth;
    const menuHeight = menu.offsetHeight;

    const windowWidth = window.innerWidth;
    const windowHeight = window.innerHeight;

    if (x + menuWidth > windowWidth) x = windowWidth - menuWidth - 10;
    if (y + menuHeight > windowHeight) y = windowHeight - menuHeight - 10;
    if (x < 10) x = 10;
    if (y < 10) y = 10;

    menu.style.left = `${x}px`;
    menu.style.top = `${y}px`;
});

document.addEventListener('click', () => {
    menu.hidden = true;
});

// Prevent clicks *inside* the menu from closing it immediately
menu.addEventListener('click', (e) => {
    e.stopPropagation();
});

// Theme Toggle
const menu_theme = document.getElementById('menu-theme');
menu_theme.addEventListener('click', () => {
    window.nextTheme();
    menu_theme.textContent = `Theme: ${window.theme}`;
});

// Star Physics Inputs
document.getElementById('input-blast-radius').addEventListener('input', (e) => {
    blastRadius = parseFloat(e.target.value) || 0;
});

document.getElementById('input-blast-strength').addEventListener('input', (e) => {
    blastStrength = parseFloat(e.target.value) || 0;
});

document.getElementById('input-pull-radius').addEventListener('input', (e) => {
    mouseInfluenceRadius = parseFloat(e.target.value) || 0;
});

// Cull Stars Action
document.getElementById('btn-cull-stars').addEventListener('click', () => {
    const starsToRemove = 50;
    for (let i = 0; i < starsToRemove; i++) {
        if (stars.length > 0) {
            stars.shift();
        }
    }
});
