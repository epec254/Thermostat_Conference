//requires eventsource (npm install eventsource)

var EventSource = require('eventsource');
 
var eventSourceInitDict = {
    headers: {Authorization: "Bearer <access token here>"}
};
 
var url = "https://api.spark.io/v1/events/";
 
var es = new EventSource(url, eventSourceInitDict);

var fs = require('fs');
var wstream = fs.createWriteStream('contest_results.csv');

 
//add your listener
es.addEventListener('contest-status', function(e) {
    var rawData = JSON.parse(e.data);
    var parsedData = JSON.parse(rawData.data);
	
	
    wstream.write(rawData.coreid + ',' + rawData.published_at + ',' + parsedData.id + "," + parsedData.team + "," + parsedData.contest_status + '\n');
    
    console.log(rawData.coreid + ',' + rawData.published_at + ',' + parsedData.id + "," + parsedData.team + "," + parsedData.contest_status + '\n'); //result [0,12.14,12.15,548.54,15,457]
});