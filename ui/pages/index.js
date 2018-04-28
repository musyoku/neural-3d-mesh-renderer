import { Component } from "react"
import * as three from "three"

export default class App extends Component {
    componentDidMount() {
        const scene = new three.Scene()
        const camera = new three.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000)

        const renderer = new three.WebGLRenderer()
        renderer.setSize(window.innerWidth, window.innerHeight)
        this.refs.renderer.appendChild(renderer.domElement)

        const geometry = new three.BoxGeometry(1, 1, 1)
        const material = new three.MeshBasicMaterial({ color: 0x00ff00 })
        const cube = new three.Mesh(geometry, material)
        scene.add(cube)

        camera.position.z = 5

        const controls = new three.OrbitControls(camera)
        controls.minDistance = 2
        controls.maxDistance = 5
        controls.enablePan = false
        controls.enableZoom = false
    }
    render() {
        return (
            <div id="app">
                <div id="renderer" ref="renderer"></div>
            </div>
        )
    }
}