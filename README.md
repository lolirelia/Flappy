A 2d multiplayer implementation of Flappy using Raylib for rendering a libuv for UDP networking. 

Gameplay Mechanics:
Hold left click to rise, release left click to fall. The objective is to be the first to complete the map. 

Map:
A map is stored in binary format as Rectangle collisions. Each Rectangle object contains an x, y, width and height. The x and y are relative to the game world. 
Simple rectangular AABB collision detection is used. 

Server:
Server also renders the gameplay(Though this isn't necessary, I chose to do it to have access to the timing functions like GetFrameTime()). 
The server can be purely terminal but another timer would need to be used in order to monitor the delta time. GetFrameTime only works with BeginDrawing and EndDrawing calls.

Gameloop:
The rendering happens at 240fps (roughly. While the physics (game update) occurs at 60fps. Every other tick (30ms) the gamestate is sent to the client.

Client:
The client also has its own tick counter (this is purely used to measure render delay)
The client receives gamestates from the server which are pushed into a container. It then decides what to render next based on the tick of the packet and the render delay.
Interpolation is used between the current render state and the next render state. 
There is a fixed 6 tick render delay (16.66666*6) roughly 100m. This means your inputs are consistently 100ms late.
Client side prediction could be implemeneted to remove this delay, I specifically chose not to do this to keep the gameclient and server simple.
100ms is plenty of time for the client to receive packets from the server.



To Build:
```
git clone https://github.com/lolirelia/Flappy.git

cd Flappy

git submodule update --init --recursive

cmake -B build

cp -r resources build/resources

cd build

make
```
Note: If on windows just copy and paste resource directory to binary output directory instead of `cp -r resources build/resources`

Two separate binaries are built. The server binary and the client binary. Server will host the game on address "0.0.0.0" and port 23456 while a client will attempt to connect to it.


If you'd like like to host the server on a separate machine , make sure to port forward which ever port you decide to use. 
Note: IF YOU DECIDED TO HOST ON A SEPARATE MACHINE - make sure to change this line `assert(uv_ip4_addr("0.0.0.0", 23456, (struct sockaddr_in*)&host) == 0);` in client.c To match the host machine's ip.


Server will need to be launched before the clients.
When desired number of players connect (kMaxNumberOfPlayers in gameshared.h) the game will begin and server will start sending gamestate packets to the clients.
By default I set this value to 1 to make testing easier. If you'd like to play with two players , change kMaxNmberOfPlaeyrs to 2 and so on..

![gameplay](https://github.com/lolirelia/Flappy/assets/50451019/c10d9670-755f-442a-b800-e6fa40604ccd)



