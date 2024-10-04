# RF Jammer App by RocketGod ☠️📡

This **RF Jammer App** for the Flipper Zero, made by **RocketGod**, is a powerful tool for jamming across multiple radio frequencies and modulation schemes. Below is an in-depth look at each mode, from its technical details to the real-world impact of its jamming strategy.

https://github.com/user-attachments/assets/4a34bc73-d419-480e-bb87-90216eb8a1e0

## 📡 Frequency Control

The app supports multiple frequency bands, ensuring compliance with the ranges handled by the Flipper's sub-GHz radio:
- **Band 1**: 300 MHz – 348 MHz
- **Band 2**: 387 MHz – 464 MHz
- **Band 3**: 779 MHz – 928 MHz

You can adjust frequencies with **precision**:
- **Left/Right** arrows move between digits to adjust.
- **Up/Down** arrows increase or decrease the selected digit.

The app will automatically correct the frequency if it's outside the valid range for the selected band.

---

## ⚙️ Jamming Modes Breakdown

Each jamming mode is implemented as a distinct modulation scheme and data pattern. The app generates these patterns and transmits them over the RF link to disrupt legitimate signals in the selected frequency range.

### 🦾 **OOK 650 kHz** (On-Off Keying):
- **Pattern**: A continuous stream of `0xFF` (i.e., all bits set to `1`, equivalent to `11111111`).
- **Mechanism**: In **OOK** (On-Off Keying), the presence or absence of a carrier wave represents binary data. In this case, the app transmits `11111111`, meaning the carrier is always "on."
- **Impact**: This mode overwhelms receivers that use OOK modulation by constantly transmitting a high state (fully "on"). Because the signal never drops, devices expecting to detect short pulses (like garage doors, remotes, etc.) will be swamped and unable to distinguish real data from the noise.

### ⚡ **2FSK 2.38 kHz** (Frequency Shift Keying):
- **Pattern**: Alternates between `0xAA` (`10101010`) and `0x55` (`01010101`), simulating binary `0`s and `1`s.
- **Mechanism**: **2FSK** modulates the frequency by shifting between two discrete frequencies. A low deviation of **2.38 kHz** means that the frequency shifts only slightly between the two states, making this mode very precise.
- **Impact**: Narrowband receivers expecting binary frequency-shifted data will receive rapid shifts between frequencies, confusing their demodulators. This small frequency shift can effectively jam simple devices, such as low-data-rate remotes, by creating ambiguity in the frequency state they expect.

### 🔥 **2FSK 47.6 kHz**:
- **Pattern**: Alternates between `0xAA` and `0x55`, just like the 2.38 kHz mode.
- **Mechanism**: Similar to the 2FSK 2.38 kHz mode, but with a much higher deviation of **47.6 kHz**. This makes the frequency shifts more pronounced, allowing the jammer to disrupt broader spectrum devices.
- **Impact**: The wider frequency deviation affects a larger bandwidth, making it effective against systems that use wider channels or higher data rates. This mode can cause severe interference across a broader frequency spectrum, jamming systems with a higher tolerance for noise or frequency shifts.

### 💥 **MSK 99.97 Kb/s** (Minimum Shift Keying):
- **Pattern**: A stream of **random data** (each byte is randomly generated, not a static pattern like the previous modes).
- **Mechanism**: **MSK** is a highly efficient modulation technique where the frequency shifts are minimal, which makes it spectrally efficient (i.e., it occupies less bandwidth). The randomness of the data simulates high-speed communication, creating a noise-like signal.
- **Impact**: By simulating a noisy digital communication channel, this mode is highly effective against digital systems that rely on MSK or similar modulation schemes. The continuous flow of random data saturates the receiver, making it impossible for legitimate data to be detected, especially in high-speed links like telemetry systems.

### 📶 **GFSK 9.99 Kb/s** (Gaussian Frequency Shift Keying):
- **Pattern**: Like the MSK mode, this also uses **random data**.
- **Mechanism**: **GFSK** is a variant of FSK where the frequency shifts are smoothed by a Gaussian filter. This reduces the bandwidth required for transmission, making it more efficient while still being robust against interference.
- **Impact**: **GFSK** is widely used in Bluetooth and low-power RF systems. This mode simulates an authentic transmission with continuous random data, making it ideal for disrupting low-power communications without requiring significant bandwidth. Devices expecting real GFSK signals will be overloaded with random frequency shifts, making proper communication impossible.

### 🚀 **Bruteforce 0xFF**:
- **Pattern**: A continuous stream of `0xFF` (equivalent to `11111111`).
- **Mechanism**: This mode sends a constant, unmodulated signal of `1`s. In the digital domain, `0xFF` means every bit is a `1`, resulting in a strong, uninterrupted carrier wave being transmitted.
- **Impact**: The **Bruteforce 0xFF** mode creates the most aggressive form of jamming. By transmitting non-stop high bits, it forces constant noise across the frequency, which jams nearly any communication within the affected band. Most RF systems rely on alternating data bits (`1`s and `0`s), so flooding the airwaves with pure `1`s causes receivers to lock up, unable to process real signals.

---

## 💣 Controls

- **Up/Down Buttons**: Modify the currently selected digit in the frequency.
- **Left/Right Buttons**: Move between digits to adjust frequency values.
- **OK Button**: Switch jamming modes in real-time.
- **Back Button**: Stop the jamming and exit the app.

---

**Disclaimer**: This app is intended for educational and research purposes by experienced RF users. Ensure compliance with local regulations before using this tool.

![rocketgod_logo](https://github.com/RocketGod-git/shodanbot/assets/57732082/7929b554-0fba-4c2b-b22d-6772d23c4a18)
