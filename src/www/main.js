// Button map (DOM id -> command string)
const buttonMap = {
    "loop-a": "/direction/loop_a",
    "loop-b": "/direction/loop_b",
    "minus-45": "/direction/min_45",
    "plus-45": "/direction/plus_45",
    "vertical": "/direction/vertical"
};

document.addEventListener("DOMContentLoaded", () => {
	Object.entries(buttonMap).forEach(([id, command]) => {
	    console.log("Add event listeners for button: ", id);
	    const el = document.getElementById(id);
	    if (el) {
		el.addEventListener("click", () => {
		    fetch(command)
		        .then(response => {
		            if (!response.ok) {
		                throw new Error(`HTTP ${response.status}`);
		            }

		            console.log(`Command sent: ${command}`);
		        })
		        .catch(error => console.error("Request failed:", error));
		});
	    }
	});
});

