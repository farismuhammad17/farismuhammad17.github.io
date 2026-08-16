const maxStarCount = 500;
const maxShootingStarCount = 3;
const shootingStarFrequency = 0.008;

let starCount = 250;

// Mouse release shockwave
let blastRadius = 350;
let blastStrength = 10.0;

// Mouse hold strength
let mousePullRadius = 220;

// Single consistent star color palette
const starPalette = {
    starColors: [
        '255, 255, 255', // White
        '173, 216, 230', // Soft blue
        '255, 253, 208', // Soft yellow
        '255, 192, 203', // Soft pink/red
        '152, 251, 152'  // Soft green
    ],
    // Dynamic or theme-aware line/shooting star colors if needed, or static
    constellationColor: '255, 255, 255',
    constellationAlpha: 0.15,
    shootingStarColor: '255, 255, 255'
};

const canvas = document.getElementById('starfield');
const ctx = canvas.getContext('2d');

let width = canvas.width = window.innerWidth;
let height = canvas.height = window.innerHeight;

// Track mouse position and click state for the Black Hole feature
let mouseX = -9999;
let mouseY = -9999;
let isMouseDown = false;

window.addEventListener('resize', () => {
    width = canvas.width = window.innerWidth;
    height = canvas.height = window.innerHeight;
});

window.addEventListener('mousemove', (e) => {
    mouseX = e.clientX;
    mouseY = e.clientY;
});

window.addEventListener('mousedown', () => {
    isMouseDown = true;
});

window.addEventListener('mouseup', () => {
    if (isMouseDown) {
        isMouseDown = false;

        stars.forEach(star => {
            let dx = star.x - mouseX;
            let dy = star.y - mouseY;
            let dist = Math.hypot(dx, dy);

            if (dist < blastRadius && dist > 0) {
                let force = (blastRadius - dist) / blastRadius;
                star.vx += (dx / dist) * force * blastStrength;
                star.vy += (dy / dist) * force * blastStrength;
            }
        });
    }
});

class Star {
    constructor() {
        this.x = Math.random() * width;
        this.y = Math.random() * height;
        this.vx = (Math.random() - 0.5) * 0.4;
        this.vy = (Math.random() - 0.5) * 0.4;

        this.baseRadius = Math.random() * 1.5 + 0.5;
        this.radius = this.baseRadius;
        this.alpha = Math.random() * 0.7 + 0.3;

        this.twinkleSpeed = Math.random() * 0.02 + 0.005;
        this.twinklePhase = Math.random() * Math.PI * 2;

        this.colorIndex = Math.floor(Math.random() * starPalette.starColors.length);
        this.history = [];
    }

    update() {
        this.twinklePhase += this.twinkleSpeed;
        this.radius = this.baseRadius + Math.sin(this.twinklePhase) * 0.25;
        if (this.radius < 0.2) this.radius = 0.2;

        this.vx += (Math.random() - 0.5) * 0.03;
        this.vy += (Math.random() - 0.5) * 0.03;
        this.vx *= 0.95;
        this.vy *= 0.95;

        this.history.push({ x: this.x, y: this.y });
        if (this.history.length > 8) {
            this.history.shift();
        }

        this.x += this.vx;
        this.y += this.vy;

        if (this.x - this.radius < 0) {
            this.x = this.radius;
            this.vx = -this.vx;
            this.history = [];
        } else if (this.x + this.radius > width) {
            this.x = width - this.radius;
            this.vx = -this.vx;
            this.history = [];
        }

        if (this.y - this.radius < 0) {
            this.y = this.radius;
            this.vy = -this.vy;
            this.history = [];
        } else if (this.y + this.radius > height) {
            this.y = height - this.radius;
            this.vy = -this.vy;
            this.history = [];
        }
    }

    draw() {
        let color = starPalette.starColors[this.colorIndex];
        let currentAlpha = this.alpha * (0.5 + 0.5 * Math.sin(this.twinklePhase));

        // Draw motion blur trail
        if (this.history.length > 1) {
            ctx.beginPath();
            ctx.moveTo(this.history[0].x, this.history[0].y);
            for (let i = 1; i < this.history.length; i++) {
                ctx.lineTo(this.history[i].x, this.history[i].y);
            }
            ctx.strokeStyle = `rgba(${color}, ${currentAlpha * 0.5})`;
            ctx.lineWidth = this.radius;
            ctx.stroke();
        }

        // Draw the star itself
        ctx.beginPath();
        ctx.arc(this.x, this.y, this.radius, 0, Math.PI * 2);
        ctx.fillStyle = `rgba(${color}, ${currentAlpha})`;
        ctx.fill();
    }
}

// Shooting Star Class
class ShootingStar {
    constructor() {
        this.reset();
    }

    reset() {
        let edge = Math.floor(Math.random() * 4);
        let speed = Math.random() * 5 + 4;

        if (edge === 0) {
            this.x = Math.random() * width;
            this.y = -50;
            let angle = Math.random() * Math.PI;
            this.vx = Math.cos(angle) * speed;
            this.vy = Math.sin(angle) * speed;
        } else if (edge === 1) {
            this.x = width + 50;
            this.y = Math.random() * height;
            let angle = Math.PI * 0.5 + Math.random() * Math.PI;
            this.vx = Math.cos(angle) * speed;
            this.vy = Math.sin(angle) * speed;
        } else if (edge === 2) {
            this.x = Math.random() * width;
            this.y = height + 50;
            let angle = Math.PI + Math.random() * Math.PI;
            this.vx = Math.cos(angle) * speed;
            this.vy = Math.sin(angle) * speed;
        } else {
            this.x = -50;
            this.y = Math.random() * height;
            let angle = -Math.PI * 0.5 + Math.random() * Math.PI;
            this.vx = Math.cos(angle) * speed;
            this.vy = Math.sin(angle) * speed;
        }

        this.length = Math.random() * 80 + 40;
    }

    update(starsList) {
        starsList.forEach(star => {
            let dx = star.x - this.x;
            let dy = star.y - this.y;
            let dist = Math.hypot(dx, dy);
            if (dist < 120 && dist > 0) {
                let force = (120 - dist) / 120;
                this.vx += (dx / dist) * force * 0.04;
                this.vy += (dy / dist) * force * 0.04;
            }
        });

        this.x += this.vx;
        this.y += this.vy;
    }

    isOutOfBounds() {
        return (
            this.x < -150 ||
            this.x > width + 150 ||
            this.y < -150 ||
            this.y > height + 150
        );
    }

    draw() {
        ctx.beginPath();
        let currentSpeed = Math.hypot(this.vx, this.vy);
        if (currentSpeed === 0) return;

        let tailX = this.x - this.vx * (this.length / currentSpeed);
        let tailY = this.y - this.vy * (this.length / currentSpeed);

        ctx.moveTo(this.x, this.y);
        ctx.lineTo(tailX, tailY);
        ctx.strokeStyle = `rgba(${starPalette.shootingStarColor}, 0.75)`;
        ctx.lineWidth = 1.5;
        ctx.stroke();
    }
}

// Generate stars and shooting stars array
const stars = [];
for (let i = 0; i < starCount; i++) {
    stars.push(new Star());
}

const shootingStars = [];

function animate() {
    ctx.clearRect(0, 0, width, height);

    // Apply basic star-to-star repulsion forces
    for (let i = 0; i < stars.length; i++) {
        for (let j = i + 1; j < stars.length; j++) {
            let dx = stars[j].x - stars[i].x;
            let dy = stars[j].y - stars[i].y;
            let dist = Math.sqrt(dx * dx + dy * dy);
            let minDist = 50;

            if (dist < minDist && dist > 0) {
                let force = (minDist - dist) / minDist;
                let fx = (dx / dist) * force * 0.02;
                let fy = (dy / dist) * force * 0.02;

                stars[i].vx -= fx;
                stars[i].vy -= fy;
                stars[j].vx += fx;
                stars[j].vy += fy;
            }
        }
    }

    // Draw constellation web connecting close neighboring stars
    let maxConstellationDist = 85;
    for (let i = 0; i < stars.length; i++) {
        for (let j = i + 1; j < stars.length; j++) {
            let dx = stars[j].x - stars[i].x;
            let dy = stars[j].y - stars[i].y;
            let dist = Math.sqrt(dx * dx + dy * dy);

            if (dist < maxConstellationDist) {
                ctx.beginPath();
                ctx.moveTo(stars[i].x, stars[i].y);
                ctx.lineTo(stars[j].x, stars[j].y);
                let lineAlpha = (1 - (dist / maxConstellationDist)) * starPalette.constellationAlpha;
                ctx.strokeStyle = `rgba(${starPalette.constellationColor}, ${lineAlpha})`;
                ctx.lineWidth = (1 - (dist / maxConstellationDist)) * 1.8;
                ctx.stroke();
            }
        }
    }

    // Update and draw each star, and handle black hole physics
    stars.forEach(star => {
        let mdx = mouseX - star.x;
        let mdy = mouseY - star.y;
        let mdist = Math.sqrt(mdx * mdx + mdy * mdy);

        if (mdist < mousePullRadius && mdist > 0) {
            let force = (mousePullRadius - mdist) / mousePullRadius;
            if (isMouseDown) {
                star.vx += (mdx / mdist) * force * 0.45;
                star.vy += (mdy / mdist) * force * 0.45;
            }
        }

        star.vx *= 0.992;
        star.vy *= 0.992;

        star.update();
        star.draw();
    });

    // Handle shooting stars
    if (Math.random() < shootingStarFrequency && shootingStars.length < maxShootingStarCount) {
        shootingStars.push(new ShootingStar());
    }

    for (let i = shootingStars.length - 1; i >= 0; i--) {
        shootingStars[i].update(stars);
        shootingStars[i].draw();

        if (shootingStars[i].isOutOfBounds()) {
            shootingStars.splice(i, 1);
        }
    }

    requestAnimationFrame(animate);
}

animate();

// Add an event listener to spawn a star wherever the user clicks
window.addEventListener('click', (e) => {
    const newStar = new Star();
    newStar.x = e.clientX;
    newStar.y = e.clientY;
    newStar.vx = (Math.random() - 0.5) * 3.5;
    newStar.vy = (Math.random() - 0.5) * 3.5;
    newStar.baseRadius = Math.random() * 2.5 + 1.5;
    newStar.alpha = 1.0;
    newStar.twinklePhase = 0;

    stars.push(newStar);

    if (stars.length > maxStarCount) {
        stars.shift();
    }
});
