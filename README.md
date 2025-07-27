# Hermes Dyforge
[![License: MIT](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)  
![Platform: Windows](https://img.shields.io/badge/platform-Windows-blue)  
![Languages: C | C++ | Lua | Python](https://img.shields.io/badge/languages-C%2FC%2B%2B%2C%20Lua%2C%20Python-yellow)
[![Github All Releases](https://img.shields.io/github/downloads/seregonwar/Hermes/total.svg)]()
---

**Hermes Dyforge** is an advanced modular suite for dynamic process analysis, designed to provide powerful and flexible tools useful in contexts such as **reverse engineering**, **debugging**, **runtime monitoring**, and **security testing**.

The suite consists of three main modules, each with a well-defined role but interoperable for seamless integration.

---

## 🧩 Main Components

### 🔧 Dyforge

> **Control interface** for activating and managing DyMain features.

- Handles direct communication with `DyMain`.
- Allows activation of **runtime flags** to enable or disable advanced features.
- Can be used in **interactive mode** or integrated into other tools.
- Required to fully utilize the advanced capabilities of Hermes Dyforge.

> **Note:** `DyMain` can also run in standalone mode for simplified use cases.

---

### 💉 DyHexInject

> **Code injector** designed for stability, performance, and native compatibility.

- Identifies active processes by **PID** and **name**.
- Injects `DyMain` into the target process.
- Written entirely in **C**, for direct communication with the operating system and reduced error surface.

---

### 🧠 DyMain

> The **core engine** of the suite: a highly specialized dynamic library.

- Monitors the behavior of the target process in real time.
- Starts a **local server** for remote access to collected data.
- Supports **runtime scripting** in **Lua** and **Python**, executable directly within the process context.
- Compatible with multithreaded architectures and complex environments.

---

## 📂 Project Architecture

```
HermesDyforge/
├── Dyforge/          # User interface / CLI for DyMain management
├── DyHexInject/      # DLL injector written in C
├── DyMain/           # Main dynamic library with server and scripting
├── scripts/          # Lua and Python script examples
└── README.md         # Documentation
```

---

## ⚙️ Requirements

- 🖥️ **Operating System:** Windows 10/11 (x64)
- 🛠️ **Compiler:** GCC / MinGW / MSVC (for DyHexInject)
- 🐍 **Scripting Environment:** Python ≥ 3.7, Lua ≥ 5.3
- 🔐 **Permissions:** Administrator privileges required for injection

---

## 🚀 Quick Start Guide

```bash
# 1. Start DyHexInject
./DyHexInject.exe

# 2. Select the target process from prompt or list

# 3. Inject DyMain
# (automatically handled by DyHexInject)

# 4. Access the local server (optional)
http://localhost:PORT
```

---

## 📜 License

This project is licensed under the **MIT** License. See the [`LICENSE`](./LICENSE) file for details.  
> ❗ **Restrictions:** Commercial use requires explicit authorization. Redistribution is prohibited unless formally approved.

---

## 🧠 Contribute

If you’d like to contribute to the development of Hermes Dyforge:

- Fork the project  
- Create a dedicated branch (`feature/your-feature`)  
- Submit a detailed pull request  
- Add tests and documentation if needed
