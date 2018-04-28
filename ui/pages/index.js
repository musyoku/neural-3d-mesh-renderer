import { Component } from "react"
import * as three from "three"
import OrbitControls from "three-orbitcontrols"

export default class App extends Component {
    componentDidMount() {
        const scene = new three.Scene()
        const camera = new three.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000)

        const renderer = new three.WebGLRenderer()
        renderer.setClearColor(0xe0e0e0)
        renderer.setPixelRatio(window.devicePixelRatio)
        renderer.setSize(window.innerWidth, window.innerHeight)
        this.refs.renderer.appendChild(renderer.domElement)

        // const ambient_light = new three.AmbientLight(0xFFFFFF, 0.1)
        // scene.add(ambient_light)

        // const directional_light = new three.DirectionalLight(0xFFFFFF, 1.0)
        // scene.add(directional_light)

        // const directional_light = new three.HemisphereLight(0xaaaaaa, 0x444444)
        // scene.add(directional_light)

        const geometry = new three.BoxGeometry(1, 1, 1)
        const material = new three.MeshStandardMaterial({
            "color": new three.Color().setHSL(1, 1, 0.75),
            "roughness": 0.5,
            "metalness": 0,
            "flatShading": true
        })
        const cube = new three.Mesh(geometry, material)
        scene.add(cube)

        // scene.add(new three.HemisphereLight(0xaaaaaa, 0x444444))
        const light = new three.DirectionalLight(0xFFFFFF, 1.0)
        light.position.copy(camera.position)
        scene.add(light)

        camera.position.z = 5

        const controls = new OrbitControls(camera)
        controls.minDistance = 2
        controls.maxDistance = 5
        controls.enablePan = false
        controls.enableZoom = true

        const animate = () => {
            controls.update()
            light.position.copy(camera.position)
            renderer.render(scene, camera)
            requestAnimationFrame(animate)
        }
        animate()

    }
    render() {
        return (
            <div id="app">
                <div id="renderer" ref="renderer"></div>
            </div>
        )
    }
}