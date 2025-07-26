const Logger = require("@ptkdev/logger");
const logger = new Logger();
const inquirer = require("inquirer");

let expr = /[0-9A-Fa-f]{6}/g;

function isValidKey(a) {
	return expr.test(a) && a.length == 32;
}

function failed() {
	logger.error("Failed!");
	logger.error("The Keys You Provided Are Invalid...");
}

console.clear();
logger.info("Asking for AES Key/Secret...");
inquirer.prompt({
	type: "input",
	name: "value",
	message: "AES Key/Secret:"
}).then(key => {
	let secretKey = key.value;
	if (isValidKey(secretKey)) {
		logger.info("Asking for Initialization Vector...");
		inquirer.prompt({
			type: "input",
			name: "value",
			message: "Initialization Vector:"
		}).then(iv => {
			let ivKey = iv.value;
			if (isValidKey(ivKey)) {
				let b64One = Buffer.from(secretKey, "hex").toString("base64");
				let b64Two = Buffer.from(ivKey, "hex").toString("base64");
				
				logger.info(`Paste These In The "key" And "iv" Variables Of The Board Code!`);
				logger.debug(b64One);
				logger.debug(b64Two);
			} else {
				failed();
			}
		}).catch(e => {
			failed();
		});
	} else {
		failed();
	}
}).catch(e => {
	failed();
});