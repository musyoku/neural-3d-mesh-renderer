import { Component } from "react"
import * as three from "three"
import OrbitControls from "three-orbitcontrols"
import WebSocketClient from "../websocket"
import enums from "../enums"
import blob_to_buffer from "blob-to-buffer"
import styled from 'styled-components'

export default class App extends Component {
    constructor(props) {
        super(props)
        this.state = {
            "top_silhouette": {
                "width": 128,
                "height": 128,
            }
        }
    }
    initScene = (vertices, faces) => {
        const scene = new three.Scene()
        const camera = new three.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000)

        const renderer = new three.WebGLRenderer()
        renderer.setClearColor(0xe0e0e0)
        renderer.setPixelRatio(window.devicePixelRatio)
        renderer.setSize(window.innerWidth * 0.7, window.innerHeight)
        this.refs.renderer.appendChild(renderer.domElement)

        const geometry = this.buildGeometry(vertices, faces)
        const mesh = this.buildMesh(geometry)
        const wire = this.buildWire(geometry)
        scene.add(mesh)
        scene.add(wire)

        const lights = [
            new three.HemisphereLight(0x999999, 0x000000),
            new three.DirectionalLight(0xFFFFFF, 0.5)
        ]
        for (const light of lights) {
            light.position.copy(camera.position)
            scene.add(light)
        }

        scene.add(new three.AxesHelper(20))

        const controls = new OrbitControls(camera)
        controls.minDistance = 2
        controls.maxDistance = 5
        controls.enablePan = false
        controls.enableZoom = true

        this.controls = controls
        this.lights = lights
        this.scene = scene
        this.camera = camera
        this.renderer = renderer
        this.geometry = geometry
        this.mesh = mesh
        this.wire = wire

        this.animate()
    }
    buildGeometry = (vertices, faces) => {
        const geometry = new three.Geometry()
        // 頂点を追加
        for (const vertex of vertices) {
            geometry.vertices.push(new three.Vector3(vertex[0], vertex[1], vertex[2]))
        }
        // 面を追加
        for (const face of faces) {
            geometry.faces.push(new three.Face3(face[0], face[1], face[2]))
        }
        geometry.computeFaceNormals()
        return geometry
    }
    buildWire = (geometry) => {
        const material = new three.MeshBasicMaterial({ "color": 0xffffff, "wireframe": true })
        return new three.Mesh(geometry, material)
    }
    buildMesh = (geometry) => {
        const material = new three.MeshStandardMaterial({
            "color": new three.Color().setHSL(1, 1, 0.75),
            "roughness": 0.5,
            "metalness": 0,
            "flatShading": true,
        })
        return new three.Mesh(geometry, material)
    }
    animate = () => {
        const { controls, lights, scene, camera, renderer } = this
        controls.update()
        // カメラの位置によらずライトが常に画面上部にくるようにする
        const pseudo_x = Math.sqrt(camera.position.z * camera.position.z + camera.position.x * camera.position.x)
        const light_rad = Math.atan2(camera.position.y, pseudo_x) + (30.0 / 180.0 * Math.PI)
        const light_position = new three.Vector3(camera.position.x, Math.sin(light_rad) * 5.0, camera.position.z)
        for (const light of lights) {
            light.position.copy(light_position)
        }
        renderer.render(scene, camera)
        requestAnimationFrame(this.animate)
    }
    updateVertices = (vertices) => {
        if (vertices.length !== this.geometry.vertices.length) {
            alert("vertices.length !== this.geometry.vertices.length")
            return
        }
        for (let i = 0; i < vertices.length; i++) {
            const vertex = vertices[i]
            this.geometry.vertices[i].set(vertex[0], vertex[1], vertex[2])
        }
        this.geometry.verticesNeedUpdate = true
        this.geometry.elementNeedUpdate = true
        this.geometry.computeFaceNormals()
    }
    onInitObject = (buffer) => {
        let offset = 1
        const num_vertices = buffer.readInt32LE(offset)
        const vertices = []
        offset += 4
        for (let n = 0; n < num_vertices; n++) {
            const x = buffer.readFloatLE(offset)
            offset += 4
            const y = buffer.readFloatLE(offset)
            offset += 4
            const z = buffer.readFloatLE(offset)
            offset += 4
            vertices.push([x, y, z])
        }
        const num_faces = buffer.readInt32LE(offset)
        const faces = []
        offset += 4
        for (let n = 0; n < num_faces; n++) {
            const v1 = buffer.readInt32LE(offset)
            offset += 4
            const v2 = buffer.readInt32LE(offset)
            offset += 4
            const v3 = buffer.readInt32LE(offset)
            offset += 4
            faces.push([v1, v2, v3])
        }
        this.initScene(vertices, faces)
    }
    onUpdateObject = (buffer) => {
        let offset = 1
        const num_vertices = buffer.readInt32LE(offset)
        const vertices = []
        offset += 4
        for (let n = 0; n < num_vertices; n++) {
            const x = buffer.readFloatLE(offset)
            offset += 4
            const y = buffer.readFloatLE(offset)
            offset += 4
            const z = buffer.readFloatLE(offset)
            offset += 4
            vertices.push([x, y, z])
        }
        this.updateVertices(vertices)
    }
    onInitSilhouetteArea = (buffer) => {
        let offset = 1
        const width = buffer.readInt32LE(offset)
        offset += 4
        const height = buffer.readInt32LE(offset)
        offset += 4
        this.setState({
            "top_silhouette": {
                width, height
            }
        })
    }
    onUpdateTopSilhouette = (buffer) => {
        let offset = 1
        const num_pixels = buffer.readInt32LE(offset)
        offset += 4
        const height = buffer.readInt32LE(offset)
        offset += 4
        const width = buffer.readInt32LE(offset)
        offset += 4
        const vertices = []

        // データをcanvasのcontextに設定
        const canvas = new Canvas(width, height)
        const ctx = canvas.getContext("2d")

        // RGBの画素値の配列を取得
        const data = ctx.getImageData(0, 0, width, height)

        for (let p = 0; p < num_pixels; p++) {
            const value = buffer.readUInt8(offset)
            offset += 1
            const x = p % width
            const y = parseInt(Math.floor(p / width))
            const index = (y * data.width + x) * 4
            data.data[index + 0] = value    // R
            data.data[index + 1] = value    // G
            data.data[index + 2] = value    // B
            data.data[index + 3] = 1        // A
        }

        ctx.putImageData(data, 0, 0)
    }
    componentDidMount = () => {
        this.ws = new WebSocketClient("localhost", 8081)
        this.ws.addEventListener("message", (event) => {
            const { data } = event
            blob_to_buffer(data, (error, buffer) => {
                if (error) {
                    throw error
                }
                const event_code = buffer.readInt8(0)
                if (event_code === enums.event.init_object) {
                    this.onInitObject(buffer)
                }
                if (event_code === enums.event.update_object) {
                    this.onUpdateObject(buffer)
                }
                if (event_code === enums.event.update_top_image) {
                    this.onUpdateTopSilhouette(buffer)
                }
                if (event_code === enums.event.init_silhouette_area) {
                    this.onInitSilhouetteArea(buffer)
                }
            })
        })
    }
    render() {
        return (
            <div className="app">
                <style jsx global>{`
                    body {
                        padding: 0;
                        margin: 0;
                    }
                    .app { 
                        background: #000;
                        width: 100vw;
                        height: 100vh;
                        display: flex;
                        flex-direction: row;
                    }
                    .renderer {
                        flex: 1 1 auto;
                    }
                    .silhouette_area {
                        flex: 0 0 auto;
                        width: 30%;
                        display: flex;
                        flex-direction: column;
                    }
                    .silhouette_area > .silhouette {
                        flex: 1 1 auto;
                        display: block;
                        border: none;
                    }
                    `}</style>
                <div className="renderer" ref="renderer" />
                <div className="silhouette_area">
                    <div className="silhouette">
                        <canvas className="canvas" ref="top_canvas" width={this.state.top_silhouette.width} height={this.state.top_silhouette.height} />
                    </div>
                    <div className="silhouette">
                        <canvas className="canvas" ref="bottom_canvas" />
                    </div>
                </div>
            </div>
        )
    }
}