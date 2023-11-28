/*  _____   _____ ____    ___   ___ ___  ____
 * |  __ \ / ____/ __ \  |__ \ / _ \__ \|___ \
 * | |__) | |   | |  | |    ) | | | | ) | __) |
 * |  ___/| |   | |  | |   / /| | | |/ / |__ <
 * | |    | |___| |__| |  / /_| |_| / /_ ___) |
 * |_|     \_____\____/  |____|\___/____|____/
 */


#ifndef SYNCHRO_H
#define SYNCHRO_H

#include <QDebug>

#include <pcosynchro/pcosemaphore.h>
#include <pcosynchro/pcothread.h>

#include "locomotive.h"
#include "ctrain_handler.h"
#include "synchrointerface.h"
#include <QVector>

/**
 * @brief La classe Synchro implémente l'interface SynchroInterface qui
 * propose les méthodes liées à la section partagée.
 */
class Synchro final : public SynchroInterface
{

protected:

    PcoSemaphore mutex{1};
    PcoSemaphore waitingSection{0};
    PcoSemaphore waitingStation{0};
    PcoSemaphore fifo{1};
    bool isInSharedSection{false};
    int nbWaiting{0};

    int nbLoco;

    int lastArrivedNum;
    int nbWaitingAtStation;


public:

    /**
     * @brief Synchro Constructeur de la classe qui représente la section partagée.
     * Initialisez vos éventuels attributs ici, sémaphores etc.
     */
    Synchro(int nbLoco) {

        this->nbLoco = nbLoco;

    }


    /**
     * @brief access Méthode à appeler pour accéder à la section partagée
     *
     * Elle doit arrêter la locomotive et mettre son thread en attente si nécessaire.
     *
     * @param loco La locomotive qui essaie accéder à la section partagée
     */
    void access(Locomotive &loco) override {

        mutex.acquire();
        if(isInSharedSection || loco.numero() != lastArrivedNum){
            ++nbWaiting;
            mutex.release();
            loco.arreter();
            afficher_message_loco(loco.numero(),"5 minutes de retard : L'attente d'un autre train en est la cause");
            waitingSection.acquire();
            mutex.acquire();
            --nbWaiting;
        }
        afficher_message_loco(loco.numero(),"Je m'engouffre dans cette section partagée !! :O");
        loco.demarrer();
        isInSharedSection = true;
        mutex.release();
        // Exemple de message dans la console globale
        afficher_message(qPrintable(QString("The engine no. %1 accesses the shared section.").arg(loco.numero())));
    }

    /**
     * @brief leave Méthode à appeler pour indiquer que la locomotive est sortie de la section partagée
     *
     * Réveille les threads des locomotives potentiellement en attente.
     *
     * @param loco La locomotive qui quitte la section partagée
     */
    void leave(Locomotive& loco) override {

        mutex.acquire();
        isInSharedSection = false;

        if(nbWaiting > 0){
            afficher_message_loco(loco.numero(),"C'est bon poto tu peux passer ;^)");
            waitingSection.release();
        }

        mutex.release();

        // Exemple de message dans la console globale
        afficher_message(qPrintable(QString("The engine no. %1 leaves the shared section.").arg(loco.numero())));
    }

    /**
     * @brief stopAtStation Méthode à appeler quand la locomotive doit attendre à la gare
     *
     * Implémentez toute la logique que vous avez besoin pour que les locomotives
     * s'attendent correctement.
     *
     * @param loco La locomotive qui doit attendre à la gare
     */
    void stopAtStation(Locomotive& loco) override {

        loco.arreter();
        mutex.acquire();
        if(nbWaitingAtStation == nbLoco - 1){ //On est la dernière locomotive à être arrivée
            lastArrivedNum = loco.numero();
            releaseStation();
            mutex.release();
            afficher_message_loco(loco.numero(),"Je suis arrivée en dernier à la station et la priorité est à MOI :^)");
            afficher_message(qPrintable(QString("The engine no. %1 arrives at the station.").arg(loco.numero())));
        } else {
            ++nbWaitingAtStation;
            mutex.release();
            afficher_message_loco(loco.numero(),"Je suis arrivée à la station mais MINCE je dois attendre maintenant :^O");
            afficher_message(qPrintable(QString("The engine no. %1 arrives at the station.").arg(loco.numero())));
            waitingStation.acquire();
        }

        PcoThread::usleep(5000000); //On attend 5 secondes (j'ai pas trouvé d'autres fonctions mdr)
        afficher_message_loco(loco.numero(),"DEPAAAART");
        afficher_message(qPrintable(QString("The engine no. %1 leaves the station.").arg(loco.numero())));
        loco.demarrer();
        // Exemple de message dans la console globale

    }


private:
    // Méthodes privées ...

    void releaseStation() {

        while(nbWaitingAtStation){
            waitingStation.release();
            --nbWaitingAtStation;
        }

    }
    // Attribut privés ...
};


#endif // SYNCHRO_H
