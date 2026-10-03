# Theatre Seat Allocation

The project includes the original console booking menu and a browser-based box office backed by the same C seat-allocation logic.

## Run the browser app on Windows

From this folder, compile the server with MinGW GCC:

```powershell
gcc -std=c11 -Wall -Wextra main.c -o theatre-web.exe -lws2_32
```

Start the local web server:

```powershell
.\theatre-web.exe --server
```

Open [http://127.0.0.1:8080](http://127.0.0.1:8080) in your browser. Keep the terminal open while booking. Reservations are held in memory and reset when the server exits. Run `.\theatre-web.exe` without `--server` to use the original console menu.

## API

- `GET /api/seats` — list all seats and their current reservation state.
- `POST /api/reservations` — reserve a seat with a JSON body containing `seatNo` and `customerName`.
- `DELETE /api/reservations/{seatNo}` — cancel a reservation.
