let socket = null;

// Button map (DOM id -> command string)
const buttonMap = {
    "loop-a": "loop_a",
    "loop-b": "loop_b",
    "minus-45": "-45",
    "plus-45": "+45",
    "vertical": "vertical"
};

const idByCommand = Object.fromEntries(
    Object.entries(buttonMap).map(([id, cmd]) => [cmd, id])
);

function handleServerMessage(data) {
    const trimmed = String(data).trim();
    if (trimmed && idByCommand[trimmed]) {
        
    }
};

function initializeWebSocketListeners(ws) {
    socket.addEventListener("message", (event) => {
        console.log("←", event.data);
        handleServerMessage(event.data);
        return false;
    });
}

// window.addEventListener("pageshow", (event) => {
//     if (event.persisted) {
//         socket = new WebSocket(wsUri);
//         initializeWebSocketListeners(websocket);
//     }
// });

socket = new WebSocket("ws://" + location.host + "/");
// initializeWebSocketListeners(socket);

socket.onmessage = function(message) {
    console.log("←", message);
    handleServerMessage(message);
    return false;
};

socket.onopen = function(event) {
    console.log("Opened Websocket connection");
};

socket.onclose = function(event) {
    console.log("Closed websocket connection");
};

// Close the websocket when the user leaves.
// window.addEventListener("pagehide", () => {
//     if (socket) {
//         console.log("Closing websocket");
//         socket.close();
//         socket = null;
//     }
// });

function ensureConnectedThenSend(message) {
    if (socket && socket.readyState === WebSocket.OPEN) {
      socket.send(message);
      console.log("→", message);
      return;
    }
};

Object.entries(buttonMap).forEach(([id, command]) => {
    console.log("Add event listeners for button: ", id);
    const el = document.getElementById(id);
    if (el) {
        el.addEventListener("click", () => {
            ensureConnectedThenSend(command);
        });
    }
});