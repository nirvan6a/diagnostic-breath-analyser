# Diagnostic Breath Analyser

Hardware designs for an experimental Arduino-based breath analyser, with gas sensors, an OLED display and a custom enclosure.
This breath analyser helps d chronic illnesses like liver or kidney failure.

## Getting started

1. Open `BAFDA.kicad_sch` in KiCad to explore the circuit and component connections.
2. Open `Diagnostic Breath Analyser.step` and `Diagnostic Breath Analyser Bottom Cap.step` in a STEP-compatible CAD tool to inspect or adapt the enclosure.
3. Export the enclosure parts to your preferred print format when ready to fabricate.

This repository currently contains the circuit schematic and enclosure models; firmware and calibration instructions are not yet included. These files do not establish diagnostic accuracy.

## Details

The breath analyser is designed to detect liver failure and diabetes based on ammonia and acetone concentrations in breath, respectively.

The device uses MOS gas sensors to detect the concentration of each gas in ppm (parts-per-million). There are clinically established links between ppm values of these gases in breath and the mentioned diseases.

The device starts taking readings when a patient presses the "actuator" switch on the top and blows into the sensors. It then logs the results to a microSD card and outputs them to an OLED display for live viewing.

The results can be compared to the established values for diagnosis. This serves as a quick filter: if the results are below a certain threshold, the diagnosis is normal, but if they are above the threshold, there is an abnormality present and it is recommended that the patient undergo further tests/consults.

See [LICENSE](LICENSE) for reuse terms.
