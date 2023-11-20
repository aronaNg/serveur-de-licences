# serveur-de-licences

Il s’agit de simuler le fonctionnement d’un serveur de licences logicielles selon un mode 
client/serveur. Un serveur met à disposition de ses clients potentiels un nombre limité de 
licences flottantes (les jetons) tel que Nb_Licence < Nb_ClientTotal. Ce mode de gestion 
est restrictif puisqu’il est probable qu’un client soit en attente d’un jeton pour pouvoir exécuter 
le programme associé. Plusieurs clients peuvent être en concurrence pour accéder aux jetons. 
D’autre part pour faciliter l’accès au jeton, le serveur garde momentanément un historique 
puisqu’il donne la priorité aux derniers utilisateurs. Chaque numéro de licence est unique. En 
cas de redistribution du jeton à un client, le serveur propose par défaut le numéro de licence 
initial. Le serveur est garant du temps d’utilisation du logiciel. Au-delà d’un temps limite, le 
serveur interroge le client pour vérifier son utilisation. 