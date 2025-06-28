# 🌟 Jogo Isométrico - Atividade Final GB

Este projeto foi desenvolvido como atividade final da disciplina **GB**, com o objetivo de aplicar os conhecimentos adquiridos ao longo do semestre.

---

## 🎮 Sobre o Jogo

O jogo se passa em um **mapa isométrico** utilizando o modelo **Diamond** para renderização dos tiles. O objetivo do jogador é **capturar todos os tiles rosas** presentes no mapa. Ao capturar todos, o jogador vence o jogo.

### ⚠️ Regras e Condições

- ✅ **Objetivo**: andar até os tiles rosas para capturá-los.
- 🧱 **Pedras**: tiles de pedra não são caminháveis. O jogador permanecerá parado se tentar ir até um deles.
- 🔥🌊 **Lava e Água**: são tiles mortais. Ao pisar sobre um deles, o personagem morre e o jogo termina.
- 🧍 **Personagem**: o jogador é representado por um sombreamento na cor roxa.

---

## 🧩 Desenvolvimento

O jogo foi implementado em C++ com OpenGL, utilizando o sistema de renderização 2D isométrico estudado durante a disciplina. O código foi baseado nos exercícios e exemplos disponibilizados no Moodle.

---

## 🛠️ Dificuldades Encontradas

Devido a limitações técnicas durante o desenvolvimento:

- Ainda **não foi possível renderizar o personagem** como uma textura separada.
- Os **objetos capturáveis** também não estão sendo renderizados com sprites próprios.
- Como alternativa, **tiles diferenciados** foram usados para representar temporariamente o personagem e os alvos capturáveis.

---

## 🎥 Demo do Jogo

Você pode conferir a demonstração do jogo [clicando aqui](https://asavbrm-my.sharepoint.com/:v:/g/personal/pedroaraujo1_edu_unisinos_br/EWDVCByiCYtEtQDWMpPDF7YBv5kWFIKhdIQLSvUEtmcRhQ?nav=eyJyZWZlcnJhbEluZm8iOnsicmVmZXJyYWxBcHAiOiJPbmVEcml2ZUZvckJ1c2luZXNzIiwicmVmZXJyYWxBcHBQbGF0Zm9ybSI6IldlYiIsInJlZmVycmFsTW9kZSI6InZpZXciLCJyZWZlcnJhbFZpZXciOiJNeUZpbGVzTGlua0NvcHkifX0&e=KzPiWs).

---

## 📁 Estrutura do Projeto

- `TileMap.h` / `TileMap.cpp` — controle de tiles e mapa
- `DiamondView.h` — projeção isométrica
- `main.cpp` — lógica principal do jogo e renderização
- `assets/` — imagens e tilesets
- `config/` — mapas e definição de tiles especiais

---

## ✅ Créditos

Desenvolvido por **Pedro Tassinari** como parte da disciplina de GB.

