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

        const geometry = new three.Geometry()

        geometry.vertices.push(new three.Vector3(0, 0, 1))
        geometry.vertices.push(new three.Vector3(1, 0, 0))
        geometry.vertices.push(new three.Vector3(0, -1, 0))
        geometry.vertices.push(new three.Vector3(-1, 0, 0))
        geometry.vertices.push(new three.Vector3(0, 1, 0))
        geometry.vertices.push(new three.Vector3(0, 0, -1))

        geometry.faces.push(new three.Face3(0, 2, 1))
        geometry.faces.push(new three.Face3(0, 3, 2))
        geometry.faces.push(new three.Face3(0, 4, 3))
        geometry.faces.push(new three.Face3(0, 1, 4))
        geometry.faces.push(new three.Face3(5, 1, 2))
        geometry.faces.push(new three.Face3(5, 2, 3))
        geometry.faces.push(new three.Face3(5, 3, 4))
        geometry.faces.push(new three.Face3(5, 4, 1))

        geometry.computeFaceNormals();

        // const geometry = new three.BoxGeometry(1, 1, 1)
        const material = new three.MeshStandardMaterial({
            "color": new three.Color().setHSL(1, 1, 0.75),
            "roughness": 0.5,
            "metalness": 0,
            "flatShading": true
        })
        const cube = new three.Mesh(geometry, material)
        scene.add(cube)

        const lights = [
            new three.HemisphereLight(0x999999, 0x000000),
            new three.DirectionalLight(0xFFFFFF, 0.5)
        ]
        for(const light of lights){
            light.position.copy(camera.position)
            scene.add(light)
        }

        scene.add(new three.AxesHelper(20))

        const controls = new OrbitControls(camera)
        controls.minDistance = 2
        controls.maxDistance = 5
        controls.enablePan = false
        controls.enableZoom = true

        const animate = () => {
            controls.update()
            // カメラの位置によらずライトが常に画面上部にくるようにする
            const pseudo_x = Math.sqrt(camera.position.z * camera.position.z + camera.position.x * camera.position.x)
            const light_rad = Math.atan2(camera.position.y, pseudo_x) + 60.0 / 180.0
            const light_position = new three.Vector3(camera.position.x, Math.sin(light_rad) * 5.0, camera.position.z)
            for (const light of lights) {
                light.position.copy(light_position)
            }
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