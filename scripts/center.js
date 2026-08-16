document.addEventListener("DOMContentLoaded", () => {
    // --- About section ---
    const birthDate = new Date(2009, 9, 17);
    const today = new Date();

    let age = today.getFullYear() - birthDate.getFullYear();
    const monthDifference = today.getMonth() - birthDate.getMonth();

    if (monthDifference < 0 || (monthDifference === 0 && today.getDate() < birthDate.getDate())) {
        age--;
    }

    const ageElement = document.getElementById("age");
    if (ageElement) {
        ageElement.textContent = age;
    }

    // --- Achievements ---
    const achievementItems = document.querySelectorAll(".achievement-item[data-target-pane]");
    const backButtons = document.querySelectorAll(".back-btn[data-target-pane]");
    const navPanes = document.querySelectorAll(".nav-pane");

    function switchToPane(paneId) {
        navPanes.forEach(pane => pane.classList.remove("active"));
        const target = document.getElementById(paneId);
        if (target) {
            target.classList.add("active");
        }
    }

    achievementItems.forEach(item => {
        item.addEventListener("click", () => {
            const targetPane = item.getAttribute("data-target-pane");
            switchToPane(targetPane);
        });
    });

    backButtons.forEach(btn => {
        btn.addEventListener("click", () => {
            const targetPane = btn.getAttribute("data-target-pane");
            switchToPane(targetPane);
        });
    });
});
