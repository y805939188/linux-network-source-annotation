const net = require('net');
const server = new net.Server();
server.listen(8090, function () {
  console.log(`此处监听8090端口`);
});
server.on('connection', function (socket) {
  console.log(socket);
  //对socket做出处理
});
server.on('listening', () => {
  console.log('listening-开始监听');
});
server.on('close', () => {
  console.log('close-关闭服务');
});
server.on('error', (err) => {
  console.log('close-关闭服务');
});