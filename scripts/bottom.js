document.addEventListener("DOMContentLoaded", () => {
    const tabButtons = document.querySelectorAll(".tab-btn");
    const tabPanes = document.querySelectorAll(".tab-pane");

    tabButtons.forEach(button => {
        button.addEventListener("click", () => {
            tabButtons.forEach(btn => btn.classList.remove("active"));
            tabPanes.forEach(pane => pane.classList.remove("active"));

            button.classList.add("active");
            const targetPane = document.getElementById(button.dataset.target);
            if (targetPane) {
                targetPane.classList.add("active");
            }
        });
    });

    const audio = document.getElementById("bg-audio");
    const playStatus = document.getElementById("play-status");
    const volumeSlider = document.getElementById("volume-slider");
    const canvas = document.getElementById("visualizer");

    let audioCtx = null;
    let analyser = null;
    let dataArray = null;
    let isVisualizerInitialized = false;

    function setupVisualizer() {
        if (isVisualizerInitialized || !audio || !canvas) return;

        try {
            audioCtx = new (window.AudioContext || window.webkitAudioContext)();
            analyser = audioCtx.createAnalyser();

            // Adjust line density here (must be power of 2: 128, 256, 512)
            analyser.fftSize = 256;

            const source = audioCtx.createMediaElementSource(audio);
            source.connect(analyser);
            analyser.connect(audioCtx.destination);

            const bufferLength = analyser.frequencyBinCount;
            dataArray = new Uint8Array(bufferLength);
            const canvasCtx = canvas.getContext("2d");

            isVisualizerInitialized = true;

            function draw() {
                requestAnimationFrame(draw);

                if (analyser && dataArray) {
                    analyser.getByteFrequencyData(dataArray);
                }

                canvasCtx.clearRect(0, 0, canvas.width, canvas.height);

                let barWidth = canvas.width / bufferLength;
                let centerY = canvas.height / 2;

                canvasCtx.fillStyle = "rgba(255, 255, 255, 0.9)";

                // Draw across the full width, reversing the index order
                // so the heavy bass frequencies land on the right side
                for (let i = 0; i < bufferLength; i++) {
                    let dataIndex = bufferLength - 1 - i; // Reverses spectrum (bass on right)
                    let normalizedValue = dataArray ? (dataArray[dataIndex] / 255) : 0;
                    let barHeight = normalizedValue * canvas.height;

                    let y = centerY - (barHeight / 2);
                    let x = i * barWidth;

                    canvasCtx.fillRect(x, y, Math.max(1, barWidth - 0.5), barHeight);
                }
            }
            draw();
        } catch (e) {
            console.warn("Web Audio initialization error:", e);
        }
    }

    if (audio) {
        audio.volume = volumeSlider ? volumeSlider.value : 0.2;

        const initAndPlay = () => {
            if (!audioCtx) {
                setupVisualizer();
            }
            if (audioCtx && audioCtx.state === "suspended") {
                audioCtx.resume();
            }
            audio.play().then(() => {
                if (playStatus) playStatus.textContent = "Stream Active";
            }).catch(err => {
                if (playStatus) playStatus.textContent = "Click anywhere to enable audio";
            });
        };

        audio.play().then(() => {
            if (playStatus) playStatus.textContent = "Stream Active";
            setupVisualizer();
        }).catch(() => {
            if (playStatus) playStatus.textContent = "Click anywhere to enable audio";

            const unlockAudio = () => {
                initAndPlay();
                window.removeEventListener("click", unlockAudio);
            };
            window.addEventListener("click", unlockAudio);
        });

        if (volumeSlider) {
            volumeSlider.addEventListener("input", (e) => {
                audio.volume = e.target.value;
            });
        }
    }
});
