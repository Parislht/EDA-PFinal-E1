#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#endif

#include "httplib.h"
#include "FMIndex.hpp"
#include "json_export.hpp"
#include <iostream>
#include <cctype>

using namespace std;

int main() {
    httplib::Server svr;
    FMIndex fm;              // vive entre peticiones, guarda el indice actual

    // Endpoint de prueba: confirma que el servidor responde
    svr.Get("/ping", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("pong", "text/plain");
    });


    // POST /build : recibe una cadena en el body, valida que sea ADN (ACGT),
    // construye el FM-Index y devuelve las estructuras finales como JSON.
    svr.Post("/build", [&](const httplib::Request& req, httplib::Response& res) {
        string texto = req.body;

        // Normalizar a mayusculas y validar el alfabeto {A,C,G,T}
        string limpio;
        for (char c : texto) {
            char u = toupper((unsigned char)c);
            if (u == 'A' || u == 'C' || u == 'G' || u == 'T')
                limpio += u;
        }

        if (limpio.empty()) {
            res.status = 400;
            res.set_content(
                "{\"error\":\"La cadena debe contener solo A, C, G, T\"}",
                "application/json");
            return;
        }

        // Construimos el indice sobre la cadena limpia (sin verbose)
        fm.build(limpio, false);

        // Serializamos y respondemos
        string json = exportState(fm);
        res.set_content(json, "application/json");
    });

    // POST /search : recibe un patron, ejecuta el backward search grabando
    // cada paso, y devuelve la traza como JSON para animar
    svr.Post("/search", [&](const httplib::Request& req, httplib::Response& res) {
        string patron;
        for (char c : req.body) {
            char u = toupper((unsigned char)c);
            if (u == 'A' || u == 'C' || u == 'G' || u == 'T')
                patron += u;
        }

        if (patron.empty()) {
            res.status = 400;
            res.set_content(
                "{\"error\":\"El patron debe contener solo A, C, G, T\"}",
                "application/json");
            return;
        }

        string json = searchTracedJson(fm, patron);
        res.set_content(json, "application/json");
    });


    svr.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type"}
    });

    if (!svr.set_mount_point("/", "../web")) {
        cout << "Advertencia: no se encontro la carpeta ../web" << endl;
    }

    cout << "Servidor corriendo en http://localhost:8080" << endl;
    cout << "  GET  /ping   -> prueba de vida" << endl;
    cout << "  POST /build  -> construye indice y devuelve estructuras" << endl;
    cout << "Abre http://localhost:8080 en tu navegador." << endl;

    svr.listen("localhost", 8080);
    return 0;
}