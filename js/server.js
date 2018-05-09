const express = require("express")
const dev = process.env.NODE_ENV !== "production"
const next = require("next")
const app = next({ dev })
const handle = app.getRequestHandler()
const port_http = 8080
const port_websocket = 8081
const enums = require("./enums")

const WebSocketServer = require("ws").Server
const wss = new WebSocketServer({ "port": port_websocket })
wss.on("connection", (ws) => {
    ws.on("message", (message) => {
        console.log("received: %s", message)
    })
})

app.prepare().then(() => {
    const server = express()
    server.post("/silhouette/init_object", (req, res) => {
        // websocketで送られてくるデータの種類を表す整数値を先頭に入れる
        const data = [Buffer.from([enums.event.init_object])]
        // 逐次的にデータの読み出しが行われる
        req.on("data", (chunk) => {
            data.push(chunk)
        })
        // 全データを読み込んだら呼ばれる
        req.on("end", () => {
            const buffer = Buffer.concat(data)
            wss.clients.forEach(client => {
                client.send(buffer)     // バイナリデータをそのまま送る
            })
        })
        res.send({
            "success": true
        })
    })
    server.post("/silhouette/update_object", (req, res) => {
        // websocketで送られてくるデータの種類を表す整数値を先頭に入れる
        const data = [Buffer.from([enums.event.update_object])]
        // 逐次的にデータの読み出しが行われる
        req.on("data", (chunk) => {
            data.push(chunk)
        })
        // 全データを読み込んだら呼ばれる
        req.on("end", () => {
            const buffer = Buffer.concat(data)
            wss.clients.forEach(client => {
                client.send(buffer)     // バイナリデータをそのまま送る
            })
        })
        res.send({
            "success": true
        })
    })
    server.post("/silhouette/update_top_image", (req, res) => {
        // websocketで送られてくるデータの種類を表す整数値を先頭に入れる
        const data = [Buffer.from([enums.event.update_object])]
        // 逐次的にデータの読み出しが行われる
        req.on("data", (chunk) => {
            data.push(chunk)
        })
        // 全データを読み込んだら呼ばれる
        req.on("end", () => {
            const buffer = Buffer.concat(data)
            wss.clients.forEach(client => {
                client.send(buffer)     // バイナリデータをそのまま送る
            })
        })
        res.send({
            "success": true
        })
    })
    server.post("/silhouette/init_silhouette_area", (req, res) => {
        // websocketで送られてくるデータの種類を表す整数値を先頭に入れる
        const data = [Buffer.from([enums.event.init_silhouette_area])]
        // 逐次的にデータの読み出しが行われる
        req.on("data", (chunk) => {
            data.push(chunk)
        })
        // 全データを読み込んだら呼ばれる
        req.on("end", () => {
            const buffer = Buffer.concat(data)
            wss.clients.forEach(client => {
                client.send(buffer)     // バイナリデータをそのまま送る
            })
        })
        res.send({
            "success": true
        })
    })
    server.get("/silhouette", (req, res) => {
        return app.render(req, res, "/silhouette", {})
    })
    server.get("*", (req, res) => {
        return handle(req, res)
    })
    server.listen(port_http, (error) => {
        if (error) {
            throw error
        }
        console.log(`http://localhost:${port_http}`)
    })
})