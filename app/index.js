const express = require('express');
const app = express();
const http = require('http');
const server = http.createServer(app);
const { Server } = require("socket.io");
const io = new Server(server);

const filesys = require('fs');
const readData = filesys.createReadStream("/tmp/mmpsu_data_out");
readData.setEncoding('UTF8');
const writeData = filesys.createWriteStream("/tmp/mmpsu_data_in");

readData.on('data', (data) => {
    state = JSON.parse(data);
});

// BEGIN BASIC SERVER STUFF
app.get('/', (req, res) => {
    res.sendFile(__dirname + '/index.html');
});

app.get('/digital-7.ttf', (req, res) => {
    res.sendFile(__dirname + '/fonts/digital-7.ttf');
});

app.get('/digital-7-mono.ttf', (req, res) => {
    res.sendFile(__dirname + '/fonts/digital-7 (mono).ttf');
});

app.get('/style.css', (req, res) => {
    res.sendFile(__dirname + '/style.css');
})

app.use('/imgs', express.static('imgs'));


// SOCKET IO STUFF
io.on('connection', (socket) => {
    
});


server.listen(8080, () => {
    console.log('listening on *:8080');
});