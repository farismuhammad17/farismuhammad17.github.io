/*
----------------------------------------------------------------
Copyright (c) 2026 Faris Muhammad. All rights reserved.

    Unauthorized copying, modification, distribution, or use of
    this file, via any medium, is strictly prohibited without
    prior written permission.

- farismuhd.dev
----------------------------------------------------------------
*/

const canvas = document.getElementById('circuitCanvas');
const ctx = canvas.getContext('2d');

let accumulator = 0; // 8-bit integer state (0-255)
let isPulseActive = false;

const pages = document.querySelectorAll('.page');
const currentAddrSpan = document.getElementById('current-addr');

function getLayoutConfig() {
    const W = window.innerWidth;
    const unit = Math.max(38, Math.min(58, W * 0.032));
    const startX = Math.max(40, W * 0.04);

    const controlPanelStartX = Math.max(W * 0.52, startX + (unit * 6.5));

    return {
        startX: startX,
        startY: 100,
        stageH: 210,
        btnWidth: unit * 0.85,
        btnHeight: unit * 0.8,
        bitWidth: unit * 0.85,
        bitHeight: unit * 0.8,
        bitSpacing: unit * 0.92,
        controlPanelY: 50,
        decBtnX: controlPanelStartX,
        bitsStartX: controlPanelStartX + (unit * 0.85) + 8,
        gateWidth: unit * 1.6,
        gateHeight: unit * 0.9
    };
}

let cfg = getLayoutConfig();

function resizeCanvas() {
    const dpr = window.devicePixelRatio || 1;
    cfg = getLayoutConfig();

    canvas.width = Math.floor(window.innerWidth * dpr);
    canvas.height = Math.floor(Math.max(document.body.scrollHeight, 2400) * dpr);
    ctx.imageSmoothingEnabled = false;
    render();
}

window.addEventListener('resize', resizeCanvas);
window.addEventListener('scroll', render);

class Gate {
    constructor(x, y, name, logicFn, pinLayout) {
        this.x = Math.round(x);
        this.y = Math.round(y);
        this.name = name;
        this.width = Math.round(cfg.gateWidth);
        this.height = Math.round(cfg.gateHeight);
        this.logicFn = logicFn;

        this.inputs = pinLayout.inputs.map((p, index) => ({
            owner: this,
            isOutput: false,
            index: index,
            relX: p.x,
            relY: p.y,
            x: Math.round(this.x + p.x),
            y: Math.round(this.y + p.y),
            value: false
        }));

        this.outputs = pinLayout.outputs.map((p, index) => ({
            owner: this,
            isOutput: true,
            index: index,
            relX: p.x,
            relY: p.y,
            x: Math.round(this.x + p.x),
            y: Math.round(this.y + p.y),
            value: false
        }));
    }

    updatePinPositions() {
        for (const pin of this.inputs) {
            pin.x = Math.round(this.x + pin.relX);
            pin.y = Math.round(this.y + pin.relY);
        }
        for (const pin of this.outputs) {
            pin.x = Math.round(this.x + pin.relX);
            pin.y = Math.round(this.y + pin.relY);
        }
    }

    evaluate() {
        const inputValues = this.inputs.map(pin => pin.value);
        const result = this.logicFn(...inputValues);
        if (this.outputs.length > 0) {
            this.outputs[0].value = Boolean(result);
        }
    }

    draw(ctx, isActive) {
        this.updatePinPositions();

        ctx.fillStyle = '#0a0a0c';
        ctx.fillRect(this.x, this.y, this.width, this.height);

        ctx.strokeStyle = isActive ? '#ffffff' : '#33333b';
        ctx.lineWidth = 1;
        ctx.strokeRect(this.x, this.y, this.width, this.height);

        ctx.fillStyle = isActive ? '#ffffff' : '#666677';
        ctx.font = '11px Courier New, monospace';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        ctx.fillText(this.name, Math.round(this.x + this.width / 2), Math.round(this.y + this.height / 2));
    }
}

function drawOrthogonalWire(ctx, points, isActive) {
    if (points.length < 2) return;
    ctx.beginPath();
    ctx.moveTo(Math.round(points[0].x), Math.round(points[0].y));
    for (let i = 1; i < points.length; i++) {
        ctx.lineTo(Math.round(points[i].x), Math.round(points[i].y));
    }
    ctx.strokeStyle = isActive ? '#00ff88' : '#22222b';
    ctx.lineWidth = isActive ? 2 : 1;
    ctx.stroke();
}

canvas.addEventListener('click', (e) => {
    const rect = canvas.getBoundingClientRect();
    const mouseX = e.clientX - rect.left;
    const mouseY = e.clientY - rect.top;

    const decBtnBox = { x: cfg.decBtnX, y: cfg.controlPanelY, w: cfg.btnWidth, h: cfg.btnHeight };
    const incBtnX = cfg.bitsStartX + (8 * cfg.bitSpacing) + 4;
    const incBtnBox = { x: incBtnX, y: cfg.controlPanelY, w: cfg.btnWidth, h: cfg.btnHeight };

    if (mouseX >= decBtnBox.x && mouseX <= decBtnBox.x + decBtnBox.w &&
        mouseY >= decBtnBox.y && mouseY <= decBtnBox.y + decBtnBox.h) {
        accumulator = (accumulator - 1 + 256) % 256;
        triggerPulse();
        return;
    }

    if (mouseX >= incBtnBox.x && mouseX <= incBtnBox.x + incBtnBox.w &&
        mouseY >= incBtnBox.y && mouseY <= incBtnBox.y + incBtnBox.h) {
        accumulator = (accumulator + 1) % 256;
        triggerPulse();
        return;
    }

    for (let i = 0; i < 8; i++) {
        const boxX = cfg.bitsStartX + (i * cfg.bitSpacing);
        const boxY = cfg.controlPanelY;
        const boxW = cfg.bitWidth;
        const boxH = cfg.bitHeight;

        if (mouseX >= boxX && mouseX <= boxX + boxW && mouseY >= boxY && mouseY <= boxY + boxH) {
            const bitPosition = 7 - i;
            accumulator = accumulator ^ (1 << bitPosition);
            triggerPulse();
            return;
        }
    }
});

function triggerPulse() {
    isPulseActive = true;
    updateUIState();
    setTimeout(() => {
        isPulseActive = false;
        render();
    }, 150);
}

function updateUIState() {
    const hexVal = '0x' + accumulator.toString(16).padStart(2, '0').toUpperCase();

    let activeFound = false;
    pages.forEach(page => {
        const pageNum = page.getAttribute('data-page');
        if (pageNum == accumulator.toString()) {
            page.classList.add('active');
            activeFound = true;
        } else if (pageNum !== 'default') {
            page.classList.remove('active');
        }
    });

    const defaultPage = document.querySelector('.page[data-page="default"]');
    if (defaultPage) {
        if (!activeFound) {
            defaultPage.classList.add('active');
            if (currentAddrSpan) currentAddrSpan.textContent = hexVal;
        } else {
            defaultPage.classList.remove('active');
        }
    }

    render();
}

function render() {
    const dpr = window.devicePixelRatio || 1;
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    ctx.save();
    ctx.scale(dpr, dpr);

    const decBtn = { x: cfg.decBtnX, y: cfg.controlPanelY, w: cfg.btnWidth, h: cfg.btnHeight };
    const bits = Array.from({length: 8}, (_, i) => ({
        x: cfg.bitsStartX + (i * cfg.bitSpacing),
        y: cfg.controlPanelY,
        w: cfg.bitWidth,
        h: cfg.bitHeight
    }));

    const incBtnX = cfg.bitsStartX + (8 * cfg.bitSpacing) + 4;
    const incBtn = { x: incBtnX, y: cfg.controlPanelY, w: cfg.btnWidth, h: cfg.btnHeight };

    ctx.fillStyle = '#0a0a0c';
    ctx.strokeStyle = '#ffffff';
    ctx.lineWidth = 1.5;
    ctx.fillRect(decBtn.x, decBtn.y, decBtn.w, decBtn.h);
    ctx.strokeRect(decBtn.x, decBtn.y, decBtn.w, decBtn.h);

    ctx.fillStyle = '#ffffff';
    ctx.font = '15px Courier New, monospace';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText('-', Math.round(decBtn.x + decBtn.w/2), Math.round(decBtn.y + decBtn.h/2));

    for (let i = 0; i < 8; i++) {
        const box = bits[i];
        const bitVal = (accumulator >> (7 - i)) & 1;

        ctx.fillStyle = '#0a0a0c';
        ctx.strokeStyle = '#33333b';
        ctx.lineWidth = 1;
        ctx.fillRect(box.x, box.y, box.w, box.h);
        ctx.strokeRect(box.x, box.y, box.w, box.h);

        ctx.fillStyle = bitVal ? '#00ff88' : '#777788';
        ctx.font = '14px Courier New, monospace';
        ctx.fillText(bitVal.toString(), Math.round(box.x + box.w/2), Math.round(box.y + box.h/2));
    }

    ctx.fillStyle = '#0a0a0c';
    ctx.strokeStyle = '#ffffff';
    ctx.lineWidth = 1.5;
    ctx.fillRect(incBtn.x, incBtn.y, incBtn.w, incBtn.h);
    ctx.strokeRect(incBtn.x, incBtn.y, incBtn.w, incBtn.h);

    ctx.fillStyle = '#ffffff';
    ctx.font = '15px Courier New, monospace';
    ctx.fillText('+', Math.round(incBtn.x + incBtn.w/2), Math.round(incBtn.y + incBtn.h/2));

    ctx.fillStyle = '#888899';
    ctx.font = '12px Courier New, monospace';
    ctx.textAlign = 'left';
    const hexStr = '0x' + accumulator.toString(16).padStart(2, '0').toUpperCase();
    ctx.fillText(`= ${accumulator} (${hexStr})`, incBtn.x + incBtn.w + 10, incBtn.y + incBtn.h / 2);

    const startX = cfg.startX;
    const startY = cfg.startY;
    const stageH = cfg.stageH;
    let currentCin = false;

    for (let i = 0; i < 8; i++) {
        const bitA = ((accumulator >> i) & 1) === 1;
        const bitB = (i === 0 && isPulseActive);

        const sy = startY + (i * stageH);
        const gw = cfg.gateWidth;
        const gh = cfg.gateHeight;

        const xor1 = new Gate(startX + (gw * 1.3), sy, 'XOR', (a,b)=>Boolean(a)!==Boolean(b), {
            inputs: [{x:0, y:gh*0.3}, {x:0, y:gh*0.7}], outputs: [{x:gw, y:gh*0.5}]
        });

        // Top Block: Second XOR (Produces Sum S)
        const xor2 = new Gate(startX + (gw * 3.4), sy - (gh * 0.9), 'XOR', (a,b)=>Boolean(a)!==Boolean(b), {
            inputs: [{x:0, y:gh*0.3}, {x:0, y:gh*0.7}], outputs: [{x:gw, y:gh*0.5}]
        });

        // Middle Block: Top AND
        const and1 = new Gate(startX + (gw * 3.4), sy + (gh * 0.5), 'AND', (a,b)=>a&&b, {
            inputs: [{x:0, y:gh*0.3}, {x:0, y:gh*0.7}], outputs: [{x:gw, y:gh*0.5}]
        });

        // Bottom Block: Bottom AND
        const and2 = new Gate(startX + (gw * 3.4), sy + (gh * 1.9), 'AND', (a,b)=>a&&b, {
            inputs: [{x:0, y:gh*0.3}, {x:0, y:gh*0.7}], outputs: [{x:gw, y:gh*0.5}]
        });

        // OR gate (right, aligned with the two AND gates)
        const orGate = new Gate(startX + (gw * 5.2), sy + (gh * 1.2), 'OR', (a,b)=>a||b, {
            inputs: [{x:0, y:gh*0.3}, {x:0, y:gh*0.7}], outputs: [{x:gw, y:gh*0.5}]
        });

        // Full Adder Logic Evaluation
        xor1.inputs[0].value = bitA;
        xor1.inputs[1].value = bitB;
        xor1.evaluate();

        and2.inputs[0].value = bitA;
        and2.inputs[1].value = bitB;
        and2.evaluate();

        xor2.inputs[0].value = xor1.outputs[0].value;
        xor2.inputs[1].value = currentCin;
        xor2.evaluate();

        and1.inputs[0].value = xor1.outputs[0].value;
        and1.inputs[1].value = currentCin;
        and1.evaluate();

        orGate.inputs[0].value = and1.outputs[0].value;
        orGate.inputs[1].value = and2.outputs[0].value;
        orGate.evaluate();

        const targetBox = bits[7 - i];
        const bitTopX = targetBox.x + (targetBox.w / 2);
        const bitTopY = targetBox.y;
        const bitBottomY = targetBox.y + targetBox.h;

        const leftBusX = startX - 24 - (i * 5);

        // Wiring A & B to XOR1
        drawOrthogonalWire(ctx, [
            {x: bitTopX, y: bitTopY},
            {x: bitTopX, y: bitTopY - 20 - (i * 3)},
            {x: leftBusX, y: bitTopY - 20 - (i * 3)},
            {x: leftBusX, y: sy + (gh*0.3)},
            {x: xor1.x, y: xor1.inputs[0].y}
        ], bitA);

        drawOrthogonalWire(ctx, [
            {x: leftBusX, y: sy + (gh*0.7)},
            {x: xor1.x, y: xor1.inputs[1].y}
        ], bitB);

        // Wiring A & B to Bottom AND (and2)
        drawOrthogonalWire(ctx, [
            {x: leftBusX, y: sy + (gh*0.3)},
            {x: startX + (gw * 0.5), y: sy + (gh*0.3)},
            {x: startX + (gw * 0.5), y: and2.inputs[0].y},
            {x: and2.x, y: and2.inputs[0].y}
        ], bitA);

        drawOrthogonalWire(ctx, [
            {x: startX + (gw * 0.7), y: sy + (gh*0.7)},
            {x: startX + (gw * 0.7), y: and2.inputs[1].y},
            {x: and2.x, y: and2.inputs[1].y}
        ], bitB);

        // XOR1 output splitting to XOR2 (top) and AND1 (middle) inputs
        const xorOut = xor1.outputs[0];
        const branchX = xor1.x + xor1.width + 15;

        drawOrthogonalWire(ctx, [
            { x: xorOut.x, y: xorOut.y },
            { x: branchX, y: xorOut.y }
        ], xorOut.value);

        drawOrthogonalWire(ctx, [
            { x: branchX, y: xorOut.y },
            { x: branchX, y: xor2.inputs[0].y },
            { x: xor2.inputs[0].x, y: xor2.inputs[0].y }
        ], xorOut.value);

        drawOrthogonalWire(ctx, [
            { x: branchX, y: xorOut.y },
            { x: branchX, y: and1.inputs[0].y },
            { x: and1.inputs[0].x, y: and1.inputs[0].y }
        ], xorOut.value);

        // C_in routing into both XOR2 and AND1
        if (i > 0) {
            const cinBusX = xor1.x - 12;
            const cinSourceY = sy - (gh * 1.3);

            drawOrthogonalWire(ctx, [
                { x: cinBusX, y: cinSourceY },
                { x: cinBusX, y: and1.inputs[1].y }
            ], currentCin);

            drawOrthogonalWire(ctx, [
                { x: cinBusX, y: xor2.inputs[1].y },
                { x: xor2.inputs[1].x, y: xor2.inputs[1].y }
            ], currentCin);

            drawOrthogonalWire(ctx, [
                { x: cinBusX, y: and1.inputs[1].y },
                { x: and1.inputs[1].x, y: and1.inputs[1].y }
            ], currentCin);
        }

        // Wiring AND outputs to OR inputs
        drawOrthogonalWire(ctx, [
            {x: and1.outputs[0].x, y: and1.outputs[0].y},
            {x: startX + (gw * 4.6), y: and1.outputs[0].y},
            {x: startX + (gw * 4.6), y: orGate.inputs[0].y},
            {x: orGate.x, y: orGate.inputs[0].y}
        ], and1.outputs[0].value);

        drawOrthogonalWire(ctx, [
            {x: and2.outputs[0].x, y: and2.outputs[0].y},
            {x: startX + (gw * 4.6), y: and2.outputs[0].y},
            {x: startX + (gw * 4.6), y: orGate.inputs[1].y},
            {x: orGate.x, y: orGate.inputs[1].y}
        ], and2.outputs[0].value);

        // Route Sum output from XOR2 (top) to the right bus
        const busOffsetIndex = (7 - i);
        const rightBusX = startX + (gw * 7) + (busOffsetIndex * 7);

        drawOrthogonalWire(ctx, [
            {x: xor2.outputs[0].x, y: xor2.outputs[0].y},
            {x: rightBusX, y: xor2.outputs[0].y},
            {x: rightBusX, y: bitBottomY + 10 + (busOffsetIndex * 4)},
            {x: bitTopX, y: bitBottomY + 10 + (busOffsetIndex * 4)},
            {x: bitTopX, y: bitBottomY}
        ], xor2.outputs[0].value);

        // Draw blocks
        xor1.draw(ctx, bitA || bitB);
        xor2.draw(ctx, xor2.outputs[0].value);
        and1.draw(ctx, and1.outputs[0].value);
        and2.draw(ctx, and2.outputs[0].value);
        orGate.draw(ctx, orGate.outputs[0].value);

        // Cascade C_out to next stage's C_in
        currentCin = orGate.outputs[0].value;
        if (i < 7) {
            const carryOutX = orGate.outputs[0].x;
            const dropX = xor1.x - 12;
            const clearRightX = startX + (gw * 6.7);
            const dropY = sy + stageH - (gh * 1.3);

            drawOrthogonalWire(ctx, [
                {x: carryOutX, y: orGate.outputs[0].y},
                {x: clearRightX, y: orGate.outputs[0].y},
                {x: clearRightX, y: dropY},
                {x: dropX, y: dropY}
            ], currentCin);
        }
    }

    ctx.restore();
}

resizeCanvas();
updateUIState();
