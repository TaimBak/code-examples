/*!****************************************************************************
\file   SceneSystem.cpp
\author Austin Clark
\par    Course: GAM200AF22
\par    Copyright 2022 DigiPen (USA) Corporation

\brief
  SceneSystem is an interface that allows access to multiple containers of
  Scene objects. Scene objects are themsevles containers of gameplay objects
  such as the player and enviroment. Only gameplay objects that are in a Scene
  that is in the Active List of SceneSystem will be rendered to the screen and
  object components (Physics, Translation, Animation, etc.) updated each frame.

******************************************************************************/
#include "SceneSystem.h"
#include "InputSystem.h"
#include "MiscTools.h"

/* Used for Singleton method of storing engine systems  */
SceneSystem* SceneSystem::instance = nullptr;

/*!****************************************************************************
\brief
  Constructor for SceneSystem

\param type
  Assigns the type of system to the base Object class for debugging purposes.
******************************************************************************/
SceneSystem::SceneSystem()
  : ISystem("SceneSystem")
  , sceneSystemStream(nullptr)
  , currentScene(nullptr)
{
}

/*!****************************************************************************
\brief
  Destructor for SceneSystem
******************************************************************************/
SceneSystem::~SceneSystem() {}

/*!****************************************************************************
\brief
  Initialization functionality for SceneSystem
******************************************************************************/
void SceneSystem::Init()
{
  /* Creates scenes and containerizes them for use during runtime */
  Deserialize();

  /* Begins game by displaying splash screen sequence */
  LoadScene("SplashDigiPen");
}

/*!****************************************************************************
\brief
  Loads new scenes that were requested during current update loop.

\param dt
  Time between frames used in updating any applicable components.
******************************************************************************/
void SceneSystem::Update(float dt)
{

  /* Scenes are loaded into the active scene list if previously staged */
  if (stagedScenes.empty() == false)
  {
    LoadScene(stagedScenes.front());
    stagedScenes.erase(stagedScenes.begin());
  }
}

/*!****************************************************************************
\brief
  Shutdown function for SceneSystem. Systematically removes all accociated
  components and purges both Scene object containers to prevent memory leaks.
******************************************************************************/
void SceneSystem::Shutdown()
{

  /* Closes filestream used for deserialization of SceneSystem */
  delete sceneSystemStream;
  sceneSystemStream = nullptr;

  /* Purge Scene objects from Archetype List container */
  if (!archetypeList.empty())
  {
    size_t size = archetypeList.size();
    for (size_t i = 0; i < size; i++)
    {
      delete archetypeList.at(0);
      archetypeList.at(0) = nullptr;

      archetypeList.erase(archetypeList.begin());
    }
  }

  /* Purge Scene objects from Active List container */
  if (!activeList.empty())
  {
    size_t size = activeList.size();
    for (size_t i = 0; i < size; i++)
    {
      delete activeList.at(0);
      activeList.at(0) = nullptr;

      activeList.erase(activeList.begin());
    }
  }
}

/*!****************************************************************************
\brief
  Deserialize function for SceneSystem that opens a filestream and pulls data
  from a folder located in the game files. Creates Scene objects from all JSON
  files found in folder and places them into an archetype container for recall
  at a later time.
******************************************************************************/
void SceneSystem::Deserialize()
{
  /* Opens file that contains a list of all possible scenes that could be */
  /* loaded by the game client                                            */
  sceneSystemStream = new StreamSystem("Data/Scene/Scenes.json");

  /* Creates container to store scene names found in opened filestream    */
  std::vector<std::string> sceneNames;
  sceneSystemStream->Get<std::string>("name", sceneNames);

  /* Creates and deserializes Scene objects that are placed in an         */
  /* Archetype List. Scenes are added to an Active List and rendered once */
  /* they have been staged & loaded                                       */
  for (auto names : sceneNames)
  {
    Scene* newScene = new Scene();
    newScene->Deserialize("Data/Scene/" + names + ".json");

    archetypeList.push_back(newScene);
  }
}

/*!****************************************************************************
\brief
  Singleton interface function that returns the SceneSystem object. If one
  does not exist, it is created.
******************************************************************************/
SceneSystem* SceneSystem::GetInstance()
{
  if (instance == nullptr)
  {
    instance = new SceneSystem;
  }
  return instance;
}

/*!****************************************************************************
\brief
  Singleton interface function that removes an existing SceneSystem object.
******************************************************************************/
void SceneSystem::ResetInstance()
{
  if (instance)
  {
    delete instance;
  }

  instance = nullptr;
}

/*!****************************************************************************
\brief
  Indexes into the Archetype List container of scenes and return the requested 
  Scene. If it is not found in the list, nullptr is returned to signal that
  something isn't right.

\param sceneName
  Name of the requested Scene object
******************************************************************************/
Scene* SceneSystem::FindSceneArchetype(std::string sceneName)
{
  for (const auto& list : archetypeList)
  {
    if (list->GetName() == sceneName)
    {
      return list;
    }
  }
  return nullptr;
}

/*!****************************************************************************
\brief
  Indexes into the Active List container of scenes and return the requested 
  Scene. If it is not found in the list, nullptr is returned to signal that
  something isn't right.

\param sceneName
  Name of the requested Scene object
******************************************************************************/
Scene* SceneSystem::FindSceneActive(std::string sceneName)
{
  for (const auto& list : activeList)
  {
    if (list->GetName() == sceneName)
    {
      return list;
    }
  }
  return nullptr;
}

/*!****************************************************************************
\brief
  Places name of Scene object to be loaded at the end of the engine update loop

\param name
  Name of the Scene to be staged.
******************************************************************************/
void SceneSystem::StageScene(std::string name)
{
  stagedScenes.push_back(name);
}

/*!****************************************************************************
\brief
  Searches Archetype List for requested Scene object. If found, a clone of the
  object is created and placed into the Active List.

\param sceneName
  Name of the requested Scene object
******************************************************************************/
void SceneSystem::LoadScene(const std::string sceneName)
{
  /* Determine if the scene is already in the activeList */
  if (!FindSceneActive(sceneName))
  {
    /* Find the scene in the archetype list */
    Scene* newSceneArchetype = FindSceneArchetype(sceneName);

    if (newSceneArchetype)
    {
      /* Clone it into the activeList after initing it. */
      Scene* newSceneActive = new Scene(newSceneArchetype);

      newSceneActive->Init();

      activeList.push_back(newSceneActive);
    }
  }
}

/*!****************************************************************************
\brief
  Unloads all of the entities assigned to the passed scene and then removes the
  Scene object from the Active List.

\param sceneName
  Name of the requested Scene object
******************************************************************************/
void SceneSystem::UnloadScene(std::string sceneName)
{
  /* Singleton pointer used for accessing EntitySystem object containers     */
  EntitySystem* entitySystem = EntitySystem::GetInstance();

  /* Obtains the EntitySystem's container of active objects                  */
  std::vector<Entity*> activeEntityList = entitySystem->GetActiveList();

  /* Remove all active entities that are assigned to this scene              */
  size_t listsize = activeEntityList.size();
  for (size_t i = 0; i < listsize; i++)
  {
    if (activeEntityList.at(i)->GetAssignedScene() == sceneName)
    {
      activeEntityList.at(i)->Kill();
    }
  }

  /* Pop scene of of the scene activeList */
  for (size_t i = 0; i < activeList.size(); i++)
  {
    if (activeList.at(i)->GetName() == sceneName)
    {
      activeList.erase(activeList.begin() + i);
      break;
    }
  }
}