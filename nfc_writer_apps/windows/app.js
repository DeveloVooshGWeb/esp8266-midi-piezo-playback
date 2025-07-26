/***********************************************************************************************
 |  WiFi Buzzer Project!                                                                       |
 |                                                                                             |
 |  This JS File Is Meant To Run On Node.JS                                                    |
 |  It Will Communicate With The Board Flashed With My Code                                    |
 |  With The Provided .ino File Through The Serial                                             |
 |  It Will Use The RFID Sensor With The Libraries MFRC522 And NDEF                            |
 |  It Is Supposed To Turn Your RFID Keys Into NFC                                             |
 |  Keep In Mind That This Is Extremely                                                        |
 |  Experimental And I Broke One Of My                                                         |
 |  RFID Cards Formatting It's Memory                                                          |
 |  Via NFC                                                                                    |
 |                                                                                             |
 |  I Still Can't Find A Way To Reverse                                                        |
 |  From NFC To RFID Tho So Use This                                                           |
 |  At Your Own Risk!                                                                          |
 ***********************************************************************************************/
 
///////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// IMPORTS ////////////////////////////////////////////
 
const Logger = require("@ptkdev/logger");
const options = {
	language: "en",
	colors: true,
	debug: true,
	info: true,
	warning: true,
	error: true,
	write: true,
	type: "log",
	rotate: {
		size: "10M",
		encoding: "utf8",
	},
	path: {
		// remember: add string *.log to .gitignore
		debug_log: "./console.log",
		error_log: "./console.log",
		info_log: "./console.log"
	},
};
const logger = new Logger(options);

const inquirer = require("inquirer"); 
const fs = require("fs");
const { SerialPort } = require("serialport");

let portList = [];

///////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VARIABLES //////////////////////////////////////////

let startInit = false;

let board;

requestPath();

let credFile = "./wifi.credentials";
let comma = ",".charCodeAt(0);

let successCode = "þ".charCodeAt(0);
let failCode = "ý".charCodeAt(0);

///////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// MAIN CODE //////////////////////////////////////////

function getPortList() {
	return new Promise((res, rej) => {
		SerialPort.list().then((spList) => {
			for (sp in spList) {
				if (sp.hasOwnProperty("path")) {
					portList.push(sp.path);
				}
			}
			res();
		}).catch(e => {
			rej(e);
		});
	});
}

function failed(reason, extraReason) {
	logger.error("The RFID - NFC Writer Had An Error...");
	logger.error(`Error Description: "${reason}"`);
	if (Array.isArray(extraReason)) {
		if (extraReason.length > 0) {
			for (i = 0; i < extraReason.length; i++) {
				logger.error(extraReason[i]);
			}
		}
	}
	setTimeout(() => {
		process.exit(1);
	}, 1000);
}

function makeCredentialByteArray(ssid, pass) {
	let toRet = ["|".charCodeAt(0)];
	
	let ssidLen = ssid.length;
	let ssidLenStr = ssidLen.toString();
	if (ssidLen < 10) ssidLenStr = "0" + ssidLenStr;
	
	toRet = toRet.concat([ssidLenStr.charCodeAt(0), ssidLenStr.charCodeAt(1)]);
	toRet.push(comma);
	for (i = 0; i < ssid.length; i++) toRet.push(ssid.charCodeAt(i));
	toRet.push(comma);
	for (i = 0; i < pass.length; i++) toRet.push(pass.charCodeAt(i));
	toRet.push(0);
	
	return new Uint8Array(toRet);
}

function startNFCWriter() {
	logger.warning("Checking For The Credentials File...");
	
	if (fs.existsSync(credFile) && !fs.statSync(credFile).isDirectory()) {
		let credFileData = fs.readFileSync(credFile).toString();
		let wifiData = credFileData.replaceAll("\r", "").split("\n").filter(a => a.trim() != "");
		
		if (wifiData.length == 2) {
			logger.debug("Sending Wifi Credentials To Board...");
			
			let outputData = false;
			
			let writtenData = "";
			
			let stopListeningData = false;
			
			board.on("data", function(data){
				if (!stopListeningData) {
					let dataStr = data.toString();
					let firstCharCode = dataStr.charCodeAt(0);
					let lastCharCode = dataStr.charCodeAt(dataStr.length - 1);
					
					if (!outputData) {
						switch (firstCharCode) {
							case failCode:
								failed("Board Failed");
								break;
							case successCode:
								dataStr = dataStr.substring(1);
								logger.info("Written:");
								outputData = true;
								break;
							default:
								failed("Unrecognized Finish Code!");
								break;
						}
					}
					
					if (dataStr.length > 0 && outputData) {
						switch (lastCharCode) {
							case 10:
								matches1 = writtenData.match(/.{1,16}/g);
								if (matches1 == null) matches1 = [writtenData];
								matches1.map(a => {let matches2 = a.match(/.{1,2}/g); if (matches2 == null) {return a} else {return matches2.join(" ")}}).map(a => logger.debug(a));
								logger.info("Finished!");
								stopListeningData = true;
								setTimeout(() => {
									process.exit(0);
								}, 1000);
								break;
							default:
								dataStr.split("").map(a => a.charCodeAt(0).toString(16)).map(a => {if (a.length == 1) {return "0" + a} else {return a}}).map(a => writtenData += a);
								break;
						}
					}
				}
			});
			
			let credentialByteArray = makeCredentialByteArray(wifiData[0], wifiData[1]);
			board.write(credentialByteArray, (err, res) => {
				if (err) {
					failed("Failed To Send");
					return;
				}
				
				let toWrite = process.platform == "win32" ? "\r" : "";
				toWrite += "\n";
				
				fs.writeFileSync(credFile, toWrite);
				
				logger.info("Successfully Sent!");
				logger.warning("Because The Credentials Were Sent");
				logger.warning("The File Was Cleared For Security Reasons!");
				logger.info("Please Input Your NFC Tag/s...");
				
				setTimeout(() => {
					failed("Took Too Long", ["Were You Not Ready?", "Or Were You Connected To A Board With No Code?"]);
				}, 20000);
			});
		} else {
			failed("Invalid Credentials");
		}
	} else {
		failed("Does Not Exist");
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// INITIALIZATION /////////////////////////////////////


function init() {
	getPortList().then(() => {
		startNFCWriter();
	}).catch(e => {
		logger.error(e);
	});
}

function requestPath() {
	console.clear();
	inquirer.prompt({
		type: "input",
		name: "boardPath",
		message: "Board Path:"
	}).then(ans => {
		logger.info("INITIALIZING...");

		board = new SerialPort({
			baudRate: 115200,
			path: ans.boardPath
		}, (err) => {
			if (err) {
				failed(err);
			} else {
				init();
			}
		});
	});
}

setInterval(() => {}, 60000);

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////