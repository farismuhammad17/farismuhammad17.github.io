/*
----------------------------------------------------------------
Copyright (c) 2026 Faris Muhammad. All rights reserved.

    Unauthorized copying, modification, distribution, or use of
    this file, via any medium, is strictly prohibited without
    prior written permission.

- farismuhd.dev
----------------------------------------------------------------
*/

window.addEventListener("load", () => {
    const toggle = document.getElementById('sidebarToggle');
    const sidebar = document.getElementById('sidebar');
    if (!toggle || !sidebar) return;

    toggle.addEventListener('click', () => {
        const open = !sidebar.classList.contains('open');
        sidebar.classList.toggle('open', open);
        sidebar.setAttribute('aria-hidden', String(!open));
        toggle.setAttribute('aria-expanded', String(open));
    });

    sidebar.querySelectorAll('.group-toggle').forEach((btn) => {
        const sub = document.getElementById(btn.getAttribute('aria-controls'));
        if (!sub) return;

        btn.addEventListener('click', () => {
            const open = btn.getAttribute('aria-expanded') !== 'true';
            btn.setAttribute('aria-expanded', String(open));
            sub.classList.toggle('open', open);
        });
    });
});
