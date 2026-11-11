# ============================================================
# AmbienTrack - Reglas R8/ProGuard (Cumplimiento ISO 27400)
# Ofuscamiento y protección de la capa de datos telemétricos
# ============================================================

# --- Configuración general de ofuscamiento agresivo ---
-repackageclasses ''
-allowaccessmodification
-optimizationpasses 5
-mergeinterfacesaggressively
-overloadaggressively

# Preserva metadatos mínimos necesarios para Firebase (anotaciones y genéricos)
-keepattributes *Annotation*
-keepattributes Signature
-keepattributes InnerClasses,EnclosingMethod
-keepattributes RuntimeVisibleAnnotations,RuntimeInvisibleAnnotations

# --- Firebase Core ---
-keep class com.google.firebase.FirebaseApp { *; }
-keep class com.google.firebase.FirebaseOptions { *; }
-keep class com.google.firebase.internal.** { *; }
-dontwarn com.google.firebase.**
-dontwarn com.google.android.gms.**

# --- Firebase Auth ---
-keep class com.google.firebase.auth.** { *; }
-keepclassmembers class com.google.firebase.auth.FirebaseAuth { *; }

# --- Firebase Realtime Database ---
-keep class com.google.firebase.database.** { *; }
-keepclassmembers class com.google.firebase.database.DataSnapshot { *; }
-keepclassmembers class com.google.firebase.database.DatabaseReference { *; }
-keepclassmembers class com.google.firebase.database.ValueEventListener { *; }

# --- Google Play Services (transporte TLS / sincronización) ---
-keep class com.google.android.gms.internal.** { *; }

# --- Modelos de datos telemétricos (mapeo por reflexión de RTDB) ---
-keep class com.example.monitoreoambiental.model.** { *; }
-keepclassmembers class com.example.monitoreoambiental.model.** {
    <init>();
    <init>(...);
    void set*(***);
    *** get*();
}

# --- Actividades (referencias por reflexión del Manifest) ---
-keep class com.example.monitoreoambiental.LoginActivity { *; }
-keep class com.example.monitoreoambiental.MainActivity { *; }
-keep class com.example.monitoreoambiental.AmbienTrackApp { *; }

# --- Flutter/WebView no usados; advertencias de terceros silenciadas ---
-dontwarn org.conscrypt.**
-dontwarn org.bouncycastle.**
-dontwarn org.openjsse.**
