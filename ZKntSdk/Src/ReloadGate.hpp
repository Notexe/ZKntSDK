#pragma once

#include <atomic>
#include <thread>

namespace zknt {
    // Guards reentry into guest code from foreign (game) threads so the host
    // can guarantee no thread is executing inside this module before unloading
    // it.
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

        // Called from the host's loader thread; spinning is fine there.
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
