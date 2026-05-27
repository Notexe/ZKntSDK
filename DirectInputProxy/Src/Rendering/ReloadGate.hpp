#pragma once

#include <Windows.h>

#include <atomic>
#include <thread>

namespace knt {
    // Refcount + reloading flag so a controller thread can wait until no
    // other thread is executing inside the gated region before unmapping the
    // underlying module. The SDK has its own copy (zknt::ReloadGate) used to
    // drain detours; this one (knt::ReloadGate) drains SDK-supplied rendering
    // callbacks in the proxy.
    class ReloadGate {
      public:
        class [[nodiscard]] Ticket {
          public:
            explicit Ticket(ReloadGate* p_Gate) : m_Gate(p_Gate) {}

            Ticket(const Ticket&) = delete;
            Ticket& operator=(const Ticket&) = delete;

            ~Ticket() {
                if (m_Gate) {
                    --m_Gate->m_Inflight;
                }
            }

            explicit operator bool() const {
                return m_Gate != nullptr;
            }

          private:
            ReloadGate* m_Gate;
        };

        Ticket TryEnter() {
            ++m_Inflight;
            if (m_Reloading) {
                --m_Inflight;
                return Ticket{nullptr};
            }
            return Ticket{this};
        }

        void BeginReload() {
            m_Reloading = true;
            while (m_Inflight != 0) {
                std::this_thread::yield();
            }
        }

        void Reset() {
            m_Reloading = false;
            m_Inflight = 0;
        }

      private:
        std::atomic<bool> m_Reloading{false};
        std::atomic<int> m_Inflight{0};
    };
}
