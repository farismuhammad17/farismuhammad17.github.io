const slideData = [
    [ // 0: Initial slide
        {
            img: "assets/images/me.png",
            title: "Rhetorica 2026",
            desc: "It's a nice picture. Rhetorica was a coding competition, read more in the \"Achievements\" section."
        }
    ],
    [ // 1
        {
            img: "assets/images/hpe.png",
            title: "HPE CodeWars 2026",
            desc: "This is a very awkward picture, but it's the only picture of me alone there. I placed 8th position among 130-140 competing teams, read more in the \"Achievements\" section."
        }
    ],
    [ // 2
        {
            img: "assets/images/hacknexus.png",
            title: "Hacknexus 2026",
            desc: "Quality is trash (picture was taken from afar, can't blame the phone). I secured 1st position, read more in the \"Achievements\" section."
        }
    ]
];

let currentKey = 0;
let currentIndex = 0;

window.setSlideKey = function(newKey) {
    currentKey = newKey;
    currentIndex = 0;
    renderSlideImage();
};

function renderSlideImage() {
    const imgElement = document.querySelector("#right-tall img");
    const titleElement = document.getElementById("slide-title");
    const descElement = document.getElementById("slide-desc");

    if (!imgElement) return;

    const currentList = slideData[currentKey] || slideData[0];
    currentIndex = currentIndex % currentList.length;

    const currentSlide = currentList[currentIndex];

    // Update image source and card overlay text
    imgElement.src = currentSlide.img;
    if (titleElement) titleElement.textContent = currentSlide.title;
    if (descElement) descElement.textContent = currentSlide.desc;
}

// Auto-advance slideshow interval
setInterval(() => {
    const currentList = slideData[currentKey] || slideData[0];
    currentIndex = (currentIndex + 1) % currentList.length;
    renderSlideImage();
}, 5000);

// Initial render call on page load
renderSlideImage();

// Interactive Click-and-Hold Cursor-Tracking Zoom with Smooth Easing
document.addEventListener("DOMContentLoaded", () => {
    const container = document.getElementById("right-tall");
    const imgElement = container ? container.querySelector("img") : null;

    if (container && imgElement) {
        // Prevent native browser dragging
        imgElement.setAttribute("draggable", "false");
        imgElement.addEventListener("dragstart", (e) => e.preventDefault());

        // Start zoom on mouse down
        container.addEventListener("mousedown", (e) => {
            container.classList.add("is-zoomed");
            updateZoomOrigin(e);
        });

        // Track mouse movement across the image while holding down
        container.addEventListener("mousemove", (e) => {
            if (container.classList.contains("is-zoomed")) {
                updateZoomOrigin(e);
            }
        });

        // End zoom on mouse release or when leaving the container
        const stopZoom = () => {
            container.classList.remove("is-zoomed");
            imgElement.style.transformOrigin = "center center";
        };

        container.addEventListener("mouseup", stopZoom);
        container.addEventListener("mouseleave", stopZoom);

        function updateZoomOrigin(e) {
            const rect = container.getBoundingClientRect();
            const x = ((e.clientX - rect.left) / rect.width) * 100;
            const y = ((e.clientY - rect.top) / rect.height) * 100;
            imgElement.style.transformOrigin = `${x}% ${y}%`;
        }
    }
});
