const express = require("express")
const dev = process.env.NODE_ENV !== "production"
const next = require("next")
const app = next({ dev })
const handle = app.getRequestHandler();

app.prepare().then(() => {
    const server = express()
    server.get("/", (req, res) => {
        return app.render(req, res, "/", {})
    })
    server.get('*', (req, res) => {
        return handle(req, res);
    });
    server.listen(8080, (error) => {
        if (error) {
            throw error
        }
        console.log("Server ready on http://localhost:8080")
    })
})