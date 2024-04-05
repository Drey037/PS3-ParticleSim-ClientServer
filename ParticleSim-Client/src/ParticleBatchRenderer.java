import java.awt.*;
import java.util.ArrayList;

public class ParticleBatchRenderer implements Runnable {
    private ArrayList<Particle> particleList;

    private final Graphics g;

    public ParticleBatchRenderer(ArrayList<Particle> particleList, Graphics g) {
        this.particleList = particleList;
        this.g = g;
    }

    public void addNewParticles(ArrayList<Particle> newParticles) {
        particleList.addAll(newParticles);
    }

    @Override
    public void run() {
        while(true) {
            for (Particle particle: particleList) {
                particle.draw(g);
            }
        }
    }


}
