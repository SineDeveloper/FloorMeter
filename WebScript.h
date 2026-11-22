#ifndef WEB_SCRIPT_H
#define WEB_SCRIPT_H

#include <Arduino.h>

const char HTML_PART_2[] PROGMEM = R"=====(
    <script>
        const canvas = document.getElementById('offlineChart');
        const ctx = canvas.getContext('2d');
        let dataHistory = [];
        const maxPoints = 50; 
        const LIMIT_PITCH = 1.0;
        const LIMIT_ROLL = 0.5;

        function drawChart() {
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            ctx.strokeStyle = '#333'; ctx.lineWidth = 1;
            for(let i=1; i<6; i++) {
                let y = (canvas.height / 6) * i;
                ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(canvas.width, y); ctx.stroke();
            }
            ctx.strokeStyle = '#555'; ctx.lineWidth = 2;
            ctx.beginPath(); ctx.moveTo(0, canvas.height/2); ctx.lineTo(canvas.width, canvas.height/2); ctx.stroke();

            const scaleY = (val) => {
                let limit = 3.0; 
                if (val > limit) val = limit; if (val < -limit) val = -limit;
                return (canvas.height / 2) - (val * (canvas.height / 2) / limit);
            };

            ctx.lineWidth = 1.5; ctx.strokeStyle = '#eccc68';
            ctx.beginPath(); ctx.moveTo(0, scaleY(LIMIT_PITCH)); ctx.lineTo(canvas.width, scaleY(LIMIT_PITCH)); ctx.stroke();
            ctx.beginPath(); ctx.moveTo(0, scaleY(-LIMIT_PITCH)); ctx.lineTo(canvas.width, scaleY(-LIMIT_PITCH)); ctx.stroke();
            
            ctx.strokeStyle = '#ff7f50';
            ctx.beginPath(); ctx.moveTo(0, scaleY(LIMIT_ROLL)); ctx.lineTo(canvas.width, scaleY(LIMIT_ROLL)); ctx.stroke();
            ctx.beginPath(); ctx.moveTo(0, scaleY(-LIMIT_ROLL)); ctx.lineTo(canvas.width, scaleY(-LIMIT_ROLL)); ctx.stroke();

            if(dataHistory.length < 2) return;
            let stepX = canvas.width / (maxPoints - 1);

            ctx.strokeStyle = '#ff4d4d'; ctx.lineWidth = 3; ctx.beginPath();
            for(let i=0; i<dataHistory.length; i++) {
                let x = i * stepX; let y = scaleY(dataHistory[i].pitch);
                if(i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
            }
            ctx.stroke();

            ctx.strokeStyle = '#3399ff'; ctx.lineWidth = 3; ctx.beginPath();
            for(let i=0; i<dataHistory.length; i++) {
                let x = i * stepX; let y = scaleY(dataHistory[i].roll);
                if(i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
            }
            ctx.stroke();

            for(let i=0; i<dataHistory.length; i++) {
                let x = i * stepX;
                if (Math.abs(dataHistory[i].pitch) >= LIMIT_PITCH) {
                    ctx.fillStyle = '#ff4757'; ctx.beginPath();
                    ctx.arc(x, scaleY(dataHistory[i].pitch), 5, 0, 2*Math.PI); ctx.fill();
                }
                if (Math.abs(dataHistory[i].roll) >= LIMIT_ROLL) {
                    ctx.fillStyle = '#ffa502'; ctx.beginPath();
                    ctx.arc(x, scaleY(dataHistory[i].roll), 5, 0, 2*Math.PI); ctx.fill();
                }
            }
        }

        function sendCmd(cmd) {
            if (cmd === "start") {
                dataHistory = []; 
                drawChart();
            }
            fetch('/action?cmd=' + cmd)
                .then(res => res.json())
                .then(data => { 
                    toggleButtonState(data.measuring);
                })
                .catch(err => console.error("Error transmission command:", err));
        }

        function toggleButtonState(measuring) {
            document.getElementById('btn-start-id').disabled = measuring;
            document.getElementById('btn-stop-id').disabled = !measuring;
            document.getElementById('btn-recal-id').disabled = measuring; 
            updateStatusBox(measuring);
        }

        function recalibrateCmd() {
            if(confirm('Keep device still on the floor before confirming Re-Calibration.')) {
                document.getElementById('status-msg').innerText = 'CALIBRATING SENSOR...';
                fetch('/action?cmd=recal')
                    .then(() => { alert('Calibration complete!'); })
                    .catch(err => console.error("Error during calibration:", err));
            }
        }

        function updateStatusBox(measuring) {
            const msg = document.getElementById('status-msg');
            if(measuring) {
                msg.innerText = "RECORDING ACTIVE: LOGGING TELEMETRY DATA";
                msg.className = "status-box status-active";
            } else {
                msg.innerText = "RECORDING PAUSED: STANDBY MODE";
                msg.className = "status-box";
            }
        }

        function evaluateSafety(measuring, pitch, roll) {
            const panel = document.getElementById('safety-panel');
            if (!measuring) {
                panel.innerText = "SYSTEM IDLE: MONITORING INACTIVE";
                panel.className = "safety-indicator safety-idle";
                return;
            }
            let absP = Math.abs(pitch);
            let absR = Math.abs(roll);
            if (absR >= LIMIT_ROLL || absP >= LIMIT_PITCH) {
                panel.innerText = "CRITICAL: FLOOR SLOPE EXCEEDS MAST SAFETY LIMITS";
                panel.className = "safety-indicator safety-danger";
            } else if (absR >= (LIMIT_ROLL * 0.6) || absP >= (LIMIT_PITCH * 0.6)) {
                panel.innerText = "WARNING: SURFACE CURVATURE EXCEEDS SUPER FLAT STANDARDS";
                panel.className = "safety-indicator safety-warn";
            } else {
                panel.innerText = "OPERATIONAL: SURFACE CONDITION SAFE FOR HIGH-LIFT FORKLIFT";
                panel.className = "safety-indicator safety-safe";
            }
        }

        function setConfigMsg(text, ok) {
            const el = document.getElementById('config-msg');
            el.innerText = text;
            el.className = 'config-msg ' + (ok ? 'config-msg-ok' : 'config-msg-err');
        }

        function loadConfig() {
            fetch('/config')
                .then(res => res.json())
                .then(cfg => {
                    document.getElementById('cfg-front').value = cfg.front_track_mm / 10;
                    document.getElementById('cfg-rear').value = cfg.rear_arm_mm / 10;
                    document.getElementById('cfg-section').value = cfg.section_mm / 10;
                })
                .catch(err => console.error('Error loading config:', err));
        }

        function saveConfig() {
            const frontCm = parseFloat(document.getElementById('cfg-front').value);
            const rearCm = parseFloat(document.getElementById('cfg-rear').value);
            const sectionCm = parseFloat(document.getElementById('cfg-section').value);
            if (!(frontCm > 0) || !(rearCm > 0) || !(sectionCm >= 1)) {
                setConfigMsg('Geometry values are invalid (cm, section >= 1).', false);
                return;
            }
            const params = new URLSearchParams();
            params.append('front_track_mm', frontCm * 10);
            params.append('rear_arm_mm', rearCm * 10);
            params.append('section_mm', sectionCm * 10);
            setConfigMsg('Saving...');
            fetch('/config', { method: 'POST', body: params })
                .then(res => {
                    if (!res.ok) throw new Error('HTTP ' + res.status);
                    setConfigMsg('Geometry saved.', true);
                })
                .catch(err => setConfigMsg('Failed to save: ' + err.message, false));
        }

        setInterval(() => {
            fetch('/getdata')
                .then(res => res.json())
                .then(data => {
                    document.getElementById('val-dist').innerText = Math.max(0, Math.round(data.dist));
                    document.getElementById('val-pitch').innerText = data.pitch.toFixed(1);
                    document.getElementById('val-roll').innerText = data.roll.toFixed(1);
                    document.getElementById('val-zl').innerText = Math.round(data.z_left);
                    document.getElementById('val-zr').innerText = Math.round(data.z_right);
                    document.getElementById('val-zrear').innerText = Math.round(data.z_rear);
                    
                    toggleButtonState(data.measuring);
                    evaluateSafety(data.measuring, data.pitch, data.roll);

                    if(data.measuring) {
                        dataHistory.push({ pitch: data.pitch, roll: data.roll });
                        if(dataHistory.length > maxPoints) dataHistory.shift();
                        drawChart();
                    }
                })
                .catch(err => console.error("Error fetching telemetry:", err));
        }, 200); 
        drawChart();
        loadConfig();
    </script>
</body>
</html>
)=====";

#endif
