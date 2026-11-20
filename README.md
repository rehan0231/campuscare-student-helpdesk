# CampusCare Student Helpdesk

CampusCare is a small student-helpdesk application with a browser-based dashboard and a C backend. Students can submit campus queries, search by query ID, view query status, and mark queries as resolved. The backend stores query records in a text file and uses a linked list to manage them.

## Features

- Submit queries with a student name, roll number, category, and description.
- View query totals and pending/resolved counts.
- Search for a query by ID and resolve pending queries.
- Persist query records between server runs (stored locally in `queries.txt`).

## Requirements

- Windows with a C compiler that supports Winsock (for example, MinGW-w64 or MSVC).
- A web browser.

## Build and run

The backend listens on `127.0.0.1:8080`. For example, with MinGW-w64, compile `main.c` and `helpdesk.c` and link Winsock:

```powershell
gcc main.c helpdesk.c -o helpdesk.exe -lws2_32
```

Run the executable from a directory containing the paths expected by the server, then open [http://localhost:8080](http://localhost:8080).

> **Note:** The server currently serves the frontend from `frontend/index.html`, `frontend/style.css`, and `frontend/script.js`, and reads/writes data at `backend/queries.txt`. Keep those paths relative to the server's working directory when arranging the files.

## Project files

- `index.html`, `style.css`, `script.js` — frontend dashboard and query interactions.
- `backend-20261120T141450Z-1-001/backend/main.c` — program entry point.
- `backend-20261120T141450Z-1-001/backend/helpdesk.c` — HTTP server, query management, and persistence.
- `backend-20261120T141450Z-1-001/backend/helpdesk.h` — query structure and server declaration.
- `backend-20261120T141450Z-1-001/backend/queries.txt` — local query data; excluded from Git because it may contain student information.

## API

- `GET /api/queries` — return all queries as JSON.
- `POST /api/queries` — submit a URL-encoded query (`name`, `roll`, `category`, and `description`).
- `POST /api/resolve?id=<query-id>` — mark a query as resolved.
