apb = 84_000_000
max_psc = 65535


def validate(psc):
    return 0 <= psc <= 65535 

def get_min_psc():
    

    # frequency of led pulse
    fpulse = 1 / (100 / 1000)
    min_bpm = 40 / 60
    max_bpm = 220 / 60

    psc = 0
    clk = 0
    # start with 1% of max psc
    mul = 0.01
    while True:
        psc = max_psc * mul
        clk = apb / psc
        if (validate(clk / fpulse) 
            and validate(clk / min_bpm) 
            and validate(clk / max_bpm)
        ):
            break
        mul += 0.01

    # print(f"clk frequency: {clk}, psc: {psc}")
    # print(f"pscs: LED: {clk / fpulse}, min: {clk / min_bpm}, max: {clk / max_bpm}")
    return psc

def main():
    # get the minimum clock division factor that allows for all desired frequencies
    # i.e. pulse (10Hz), minimum bpm (0.66Hz), maximum bpm (3.66Hz)
    # this is returning 1966 for now.
    min_psc = get_min_psc()
    
    min_psc = 2500

    clk = apb / min_psc
    print(f"{apb / (10**6):.3f}MHz / {min_psc:.3f} = {clk:.3f}Hz", end="\n")
    print(f"LED: {clk / (1 / (100 / 1000))}\n")

    bpms = [50, 60, 80, 100, 135, 180, 220]
    for bpm in bpms:
        # convert to Hz
        f = bpm / 60
        print(f"Prescaler values for bpm: {bpm:.3f}")
        print(f"Crotchets: {clk / f:.3f}")
        print(f"Quavers: {clk / (f* 2):.3f}")
        print()

    pass

if __name__ == "__main__":
    main()