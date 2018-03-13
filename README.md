# Ragdolls
Code Snippets of my C++ Ragdoll Project as a student. This project was made in the **Overlord Engine 0.44**, which uses **DirectX 10** and **Ageia PhysX**. This is not the entire project, but these are loose snippets that showcase the more unique features. Below is an overview (with direct links) to the interesting files.

## Interesting files
- [Ragdolls](Ragdolls): the core files that make up the ragdoll in the game. It contains functionality to create the PhysX Skeleton, to go map the PhysX ragdoll to the 3D mesh and to go into and out of the ragdoll mode.
- [Enemy](Objects): implementation of the enemy, showcasing the switching between ragdoll and animated states.
- [Serializer](Manager): serializer that uses XML files to store and load the entire state of the game, including the PhysX actors' state. The serializer creates a temporarly save before it reset the game to enable a rollback whenever something goes wrong with either loading or saving.
- [ForceFields](ForceField): forcefield wrapper for the Ageia ForceField module.
