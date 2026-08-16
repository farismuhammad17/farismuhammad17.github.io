document.addEventListener("DOMContentLoaded", () => {
    const navButtons = document.querySelectorAll(".nav-btn");
    const navPanes = document.querySelectorAll(".nav-pane");

    navButtons.forEach(button => {
        button.addEventListener("click", () => {
            navButtons.forEach(btn => btn.classList.remove("active"));
            navPanes.forEach(pane => pane.classList.remove("active"));

            button.classList.add("active");
            const targetPane = document.getElementById(button.dataset.target);
            if (targetPane) {
                targetPane.classList.add("active");
            }
        });
    });
});
