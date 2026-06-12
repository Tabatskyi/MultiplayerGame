## How to run:
- idk how on windows on Linux you just open project select `Tools->Open Visual Studio Code` and it just opens, then `Run task->Practice3Editor Linux Development` and Play-In-Editor, and it just works.

## Network Roles
* **ROLE_Authority:** The machine that has official control over the actor (in our case, the Server) executes gameplay logic, e.g spawning and destroying.
* **ROLE_AutonomousProxy:** A client-controlled actor (like palayers character on his screen), has local control but sends inputs to the server authority.
* **ROLE_SimulatedProxy:** A client-controlled actor being observed by another client (like seeing another player moving around), simply replicates the state passed down by the server.

In our server-authoritative architecture, the server is the single source of truth for a game. If a client attempts to execute `Destroy()` locally, our server ignores it, and the player/actor will remain alive. By running the destroy logic exclusively on the server (`HasAuthority()` check), we ensure that once the server removes the actor, the destroy automatically replicates to all connected clients, it necessary for preventing cheating and desync between clients.
