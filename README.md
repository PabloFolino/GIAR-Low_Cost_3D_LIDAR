# GIAR-Low_Cost_3D_LIDAR
UTN-FRBA-GIAR


<p align="center">

**LIDAR 3D de bajo costo • Open Hardware • Open Source • Modular • Educativo**

Desarrollado por el **Grupo de Inteligencia Artificial y Robótica (GIAR)**

</p>

---

<p align="center">
<img src="docs/images/lidar_render.png" width="700">
</p>

---

## 📌 Descripción

**GIAR-Low_Cost_3D_LIDAR** es un proyecto cuyo objetivo es desarrollar un **sensor LIDAR 3D de bajo costo**, completamente abierto y fácilmente replicable, pensado para aplicaciones de:

- 🤖 Robótica móvil
- 🚜 Robótica agrícola
- 🚁 Vehículos autónomos
- 🎓 Educación
- 🔬 Investigación

El proyecto combina un **LIDAR 2D rotativo** con un **sensor de profundidad** montados sobre una plataforma mecánica diseñada íntegramente por GIAR, permitiendo generar **nubes de puntos 3D** con un costo significativamente inferior al de los sensores comerciales.

Todo el diseño será **Open Hardware** y el software será publicado como **Open Source**, permitiendo que cualquier institución pueda construir, modificar y mejorar el sistema.

---

# Objetivos

- Diseñar un LIDAR 3D económico.
- Utilizar componentes comerciales.
- Desarrollar una plataforma mecánica modular.
- Crear electrónica propia.
- Desarrollar firmware abierto.
- Publicar todo el proyecto bajo licencia abierta.
- Generar documentación completa para facilitar su reproducción.

---

# Características

- Sensor LIDAR 2D
- Sensor de profundidad
- Plataforma rotativa
- Encoder de alta resolución
- Electrónica propia
- Firmware dedicado
- Software de adquisición
- Generación de nube de puntos 3D
- Compatible con ROS2
- Diseño completamente modular

---

# Estado del proyecto

## Hardware

| Componente | Estado | Progreso | Observaciones |
|------------|:------:|:-------:|--------------|
| Base | 🟢 Completado | 100% | Diseño finalizado |
| Porta Base | 🟡 En desarrollo | 70% | Ajustes de impresión |
| Plataforma rotativa | 🟡 En desarrollo | 60% | Integración motor |
| Sensor LIDAR 2D | 🟢 Completado | 100% | Integrado |
| Sensor de profundidad | 🟡 En desarrollo | 75% | Pruebas |
| Electrónica | 🟢 Completado | 100% | PCB funcional |
| Alimentación | 🟡 En desarrollo | 80% | Validación |

---

## Software

| Módulo | Estado | Progreso | Observaciones |
|---------|:------:|:-------:|--------------|
| Firmware MCU | 🟡 | 70% | Control motor |
| Driver LIDAR | 🟢 | 100% | Finalizado |
| Sincronización sensores | 🟡 | 60% | Desarrollo |
| Procesamiento nube 3D | 🟡 | 60% | En desarrollo |
| Visualizador | 🟡 | 70% | Mejoras |
| ROS2 | 🟡 | 65% | Integración |

---

## Estudio de costos

| Ítem | Estado | Progreso |
|------|:------:|:-------:|
| Lista de materiales (BOM) | 🟡 | 80% |
| Estimación de costo | 🟡 | 75% |
| Comparativa comercial | 🟡 | 60% |

---

# Arquitectura del sistema

```
                  Sensor de Profundidad
                           │
                           │
                 ┌─────────┴──────────┐
                 │                    │
                 │    LIDAR 2D        │
                 │                    │
                 └─────────┬──────────┘
                           │
                    Plataforma Rotativa
                           │
                      Encoder absoluto
                           │
                    Electrónica GIAR
                           │
                 USB / Ethernet / WiFi
                           │
                      Computadora
                           │
                      ROS2 / PCL
                           │
                     Nube de puntos
```

---

# Roadmap

## Versión 0.1

- Diseño conceptual

## Versión 0.2

- Diseño CAD

## Versión 0.3

- Prototipo mecánico

## Versión 0.4

- Electrónica

## Versión 0.5

- Firmware

## Versión 0.6

- Driver

## Versión 0.7

- Primera nube 3D

## Versión 0.8

- Integración ROS2

## Versión 0.9

- Optimización

## Versión 1.0

- Publicación Open Source

---

# Licencia

Este proyecto será publicado bajo una licencia Open Source que permita su utilización, modificación y distribución.

---

# Grupo GIAR

**Grupo de Inteligencia Artificial y Robótica**

https://giar.ai

---

# Repositorio

https://github.com/PabloFolino/GIAR-Low_Cost_3D_LIDAR