const express = require("express")
const dev = process.env.NODE_ENV !== "production"
const next = require("next")
const app = next({ dev })
const handle = app.getRequestHandler()
const port = 8080

app.prepare().then(() => {
    const server = express()
    server.get("/", (req, res) => {
        return app.render(req, res, "/", {})
    })
    server.get('*', (req, res) => {
        return handle(req, res)
    })
    server.listen(port, (error) => {
        if (error) {
            throw error
        }
        console.log(`http://localhost:${port}`)
    })
})