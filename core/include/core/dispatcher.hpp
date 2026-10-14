#pragma once
#include <string>
#include <string_view>

namespace sc::core::dispatcher {

// Boucle serveur : accepte un client, traite les messages jusqu'a deconnexion,
// puis se re-armes. Bloque jusqu'a stop().
void run(const std::wstring& pipe_name);
void stop();

// Envoi d'une ligne de console vers l'UI (thread-safe).
void push_output(const std::string& level, const std::string& text);

} // namespace sc::core::dispatcher