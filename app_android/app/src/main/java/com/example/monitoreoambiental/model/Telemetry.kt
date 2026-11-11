package com.example.monitoreoambiental.model

import com.google.firebase.database.IgnoreExtraProperties
import com.google.firebase.database.PropertyName

/**
 * Modelo telemétrico AmbienTrack.
 * Las anotaciones @PropertyName garantizan el mapeo exacto contra las llaves
 * publicadas por el ESP32 en Firebase RTDB (Contrato JSON EV3).
 */
@IgnoreExtraProperties
data class Telemetry(
    @get:PropertyName("Temperatura") @set:PropertyName("Temperatura")
    var temperatura: Double = 0.0,

    @get:PropertyName("Humedad") @set:PropertyName("Humedad")
    var humedad: Double = 0.0,

    @get:PropertyName("Luz") @set:PropertyName("Luz")
    var luz: Int = 0,

    @get:PropertyName("Gas") @set:PropertyName("Gas")
    var gas: Int = 0,

    @get:PropertyName("Confort") @set:PropertyName("Confort")
    var confort: Int = 0
) {
    constructor() : this(0.0, 0.0, 0, 0, 0)
}
