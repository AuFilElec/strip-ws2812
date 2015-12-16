#Interface Web

Description: Ce répertoire contient l'interface Web pour piloter le ruban de LED WS2812 avec le sketch fournit dans ce dépôt.

Cette interface est tirée du code source Tony DiCola sous license MIT.

## Require
* [Python](http://www.python.org/) 2.x (Pas testé sur version 3.x)
* [Flask](http://flask.pocoo.org/) Un framework Python pour le Web
 * Pour l'installer, préféré l'usage de [pip](http://www.pip-installer.org/en/latest/)
 * Le fichier `get-pip.py` permet de récupérer *pip* pour Windows. Sur Linux et Mac, utilisez votre gestionnaire de paquets.

 ## Lancement
 Après avoir renseigné l'adresse IP de votre Arduino dans le fichier *server.py*, lancez la commande `python server.py`.
 Sur Windows, seulement si Python se trouve dans le PATH, vous pouvez double cliquer sur le fichier `server.py` pour le lancer.
 Finalement, rendez-vous à l'adresse [http://localhost:5000](http://localhost:5000).