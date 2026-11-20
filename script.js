
const form = document.getElementById("queryForm");

/* Submit student query */

form.addEventListener("submit", async function(event) {

    event.preventDefault();

    const data = new URLSearchParams();

    data.append("name",
        document.getElementById("name").value);

    data.append("roll",
        document.getElementById("roll").value);

    data.append("category",
        document.getElementById("category").value);

    data.append("description",
        document.getElementById("description").value);

    try {

        const response = await fetch("/api/queries", {
            method: "POST",
            headers: {
                "Content-Type":
                "application/x-www-form-urlencoded"
            },
            body: data
        });

        const result = await response.json();

        if (result.success) {

            document.getElementById("message").textContent =
                "Query submitted successfully! Query ID: "
                + result.id;

            form.reset();

            loadQueries();

        } else {

            document.getElementById("message").textContent =
                "Unable to submit query.";

        }

    } catch (error) {

        document.getElementById("message").textContent =
            "Server connection failed.";

    }

});


/* Load all queries */

async function loadQueries() {

    try {

        const response = await fetch("/api/queries");

        const queries = await response.json();

        displayTable(queries);

    } catch (error) {

        console.log("Unable to load queries.");

    }

}


/* Display queries in dashboard */

function displayTable(queries) {

    const table = document.getElementById("queryTable");

    table.innerHTML = "";

    let pendingCount = 0;
    let resolvedCount = 0;

    queries.forEach(function(query) {

        const row = document.createElement("tr");

        if (query.status === 0) {
            pendingCount++;
        } else {
            resolvedCount++;
        }

        const values = [
            query.id,
            query.name,
            query.rollNo,
            query.category,
            query.description
        ];

        values.forEach(function(value) {

            const cell = document.createElement("td");

            cell.textContent = value;

            row.appendChild(cell);

        });

        const statusCell = document.createElement("td");

        statusCell.textContent =
            query.status === 0 ? "Pending" : "Resolved";

        statusCell.className =
            query.status === 0 ? "pending" : "resolved";

        row.appendChild(statusCell);

        const actionCell = document.createElement("td");

        if (query.status === 0) {

            const button = document.createElement("button");

            button.textContent = "Resolve";

            button.addEventListener("click", function() {
                resolveQuery(query.id);
            });

            actionCell.appendChild(button);

        } else {

            actionCell.textContent = "Completed";

        }

        row.appendChild(actionCell);

        table.appendChild(row);

    });

    document.getElementById("total").textContent =
        queries.length;

    document.getElementById("pending").textContent =
        pendingCount;

    document.getElementById("resolved").textContent =
        resolvedCount;

}


/* Search query using ID */

async function searchQuery() {

    const id = document.getElementById("search").value;

    if (!id) {
        alert("Please enter a Query ID.");
        return;
    }

    try {

        const response = await fetch("/api/queries");

        const queries = await response.json();

        const result = queries.filter(function(query) {
            return query.id == id;
        });

        displayTable(result);

        if (result.length === 0) {
            alert("Query not found!");
        }

    } catch (error) {

        alert("Unable to search query.");

    }

}


/* Resolve student query */

async function resolveQuery(id) {

    try {

        const response = await fetch(
            "/api/resolve?id=" + id,
            {
                method: "POST"
            }
        );

        const result = await response.json();

        if (result.success) {

            alert("Query resolved successfully!");

            loadQueries();

        } else {

            alert("Query not found!");

        }

    } catch (error) {

        alert("Unable to connect to server.");

    }

}


/* Load queries when website opens */

loadQueries();
