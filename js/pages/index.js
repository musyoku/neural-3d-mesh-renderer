import { Component } from "react"
import * as three from "three"
import OrbitControls from "three-orbitcontrols"
import WebSocketClient from "../websocket"
import enums from "../enums"
import blob_to_buffer from "blob-to-buffer"

export default class App extends Component {
    initScene = (vertices, faces) => {
        const scene = new three.Scene()
        const camera = new three.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000)

        const renderer = new three.WebGLRenderer()
        renderer.setClearColor(0xe0e0e0)
        renderer.setPixelRatio(window.devicePixelRatio)
        renderer.setSize(window.innerWidth, window.innerHeight)
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
            })
        })
    }
    render() {
        return (
            <div id="app">
                <div id="renderer" ref="renderer"></div>
            </div>
        )
    }
}