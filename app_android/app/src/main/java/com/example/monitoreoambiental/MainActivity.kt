package com.example.monitoreoambiental

import android.graphics.Color
import android.os.Bundle
import android.util.Log
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import com.google.android.material.switchmaterial.SwitchMaterial
import com.google.firebase.database.DataSnapshot
import com.google.firebase.database.DatabaseError
import com.google.firebase.database.DatabaseReference
import com.google.firebase.database.FirebaseDatabase
import com.google.firebase.database.ValueEventListener
import java.util.Locale

class MainActivity : AppCompatActivity() {

    private lateinit var dbRef: DatabaseReference
    private var suppressListener = false

    private lateinit var tvConfort: TextView
    private lateinit var tvEstadoConfort: TextView
    private lateinit var tvTemp: TextView
    private lateinit var tvHum: TextView
    private lateinit var tvLuz: TextView
    private lateinit var tvGas: TextView
    private lateinit var swVentilador: SwitchMaterial
    private lateinit var swAlarma: SwitchMaterial

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        tvConfort = findViewById(R.id.tvConfort)
        tvEstadoConfort = findViewById(R.id.tvEstadoConfort)
        tvTemp = findViewById(R.id.tvTemperatura)
        tvHum = findViewById(R.id.tvHumedad)
        tvLuz = findViewById(R.id.tvLuz)
        tvGas = findViewById(R.id.tvGas)
        swVentilador = findViewById(R.id.swVentilador)
        swAlarma = findViewById(R.id.swAlarma)

        dbRef = FirebaseDatabase.getInstance(AmbienTrackApp.DATABASE_URL).reference

        swVentilador.setOnCheckedChangeListener { _, checked ->
            if (!suppressListener) {
                dbRef.child("ventilador_estado").setValue(checked)
                    .addOnFailureListener { e ->
                        Log.e(TAG, "Escritura denegada en ventilador_estado", e)
                    }
            }
        }
        swAlarma.setOnCheckedChangeListener { _, checked ->
            if (!suppressListener) {
                dbRef.child("alarma_estado").setValue(checked)
                    .addOnFailureListener { e ->
                        Log.e(TAG, "Escritura denegada en alarma_estado", e)
                    }
            }
        }

        dbRef.addValueEventListener(object : ValueEventListener {
            override fun onDataChange(snapshot: DataSnapshot) {
                if (!snapshot.exists()) return

                // Lecturas explícitas por nodo con cast seguro a Number:
                // un valor inesperado no cancela ni revienta el listener.
                val locale = Locale.US

                val tempNode = snapshot.child("Temperatura").value
                val humNode = snapshot.child("Humedad").value
                val luzNode = snapshot.child("Luz").value
                val gasNode = snapshot.child("Gas").value
                val confortNode = snapshot.child("Confort").value

                tvTemp.text = (tempNode as? Number)?.let {
                    String.format(locale, "%.1f °C", it.toDouble())
                } ?: "-- °C"
                tvHum.text = (humNode as? Number)?.let {
                    String.format(locale, "%.1f %%", it.toDouble())
                } ?: "-- %"
                tvLuz.text = (luzNode as? Number)?.toInt()?.toString() ?: "--"
                tvGas.text = (gasNode as? Number)?.toInt()?.toString() ?: "--"

                val confort = (confortNode as? Number)?.toInt()
                tvConfort.text = confort?.let { String.format(locale, "%d / 100", it) } ?: "-- / 100"

                when {
                    confort == null -> {
                        tvEstadoConfort.text = "ESTADO: SIN DATOS"
                        tvEstadoConfort.setTextColor(Color.parseColor("#64748B"))
                    }
                    confort >= 80 -> {
                        tvEstadoConfort.text = "ESTADO: ÓPTIMO"
                        tvEstadoConfort.setTextColor(Color.parseColor("#10B981"))
                    }
                    confort >= 50 -> {
                        tvEstadoConfort.text = "ESTADO: ACEPTABLE"
                        tvEstadoConfort.setTextColor(Color.parseColor("#F59E0B"))
                    }
                    else -> {
                        tvEstadoConfort.text = "ESTADO: CRÍTICO"
                        tvEstadoConfort.setTextColor(Color.parseColor("#EF4444"))
                    }
                }

                // Sincronización bidireccional de actuadores (evita rebote)
                suppressListener = true
                swVentilador.isChecked =
                    snapshot.child("ventilador_estado").value as? Boolean ?: false
                swAlarma.isChecked =
                    snapshot.child("alarma_estado").value as? Boolean ?: false
                suppressListener = false
            }

            override fun onCancelled(error: DatabaseError) {
                Log.e(TAG, "Listener de telemetría cancelado", error.toException())
                tvEstadoConfort.text = "Error: ${error.message}"
            }
        })
    }

    companion object {
        private const val TAG = "AmbienTrack"
    }
}
