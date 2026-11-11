package com.example.monitoreoambiental

import android.app.Application
import com.google.firebase.database.FirebaseDatabase

class AmbienTrackApp : Application() {

    override fun onCreate() {
        super.onCreate()
        // Persistencia offline: las lecturas se cachean y sincronizan
        // automáticamente al recuperar la conexión Wi-Fi.
        FirebaseDatabase.getInstance(DATABASE_URL)
            .setPersistenceEnabled(true)
    }

    companion object {
        const val DATABASE_URL =
            "https://estacion-ambiental-5b4b1-default-rtdb.firebaseio.com"
    }
}
