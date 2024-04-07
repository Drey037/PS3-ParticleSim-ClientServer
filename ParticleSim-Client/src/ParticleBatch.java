import java.util.ArrayList;

public class ParticleBatch extends Thread {
    // Particles assigned to this thread
    private ArrayList<Particle> particles;

    private final Object particleListLock = new Object();

    private int numParticles;

    private final int MAX_LOAD = 10000;

    private final int id;

    public ParticleBatch(int id) {
        this.id = id;
        this.particles = new ArrayList<>();
        this.numParticles = particles.size();
    }

    public ArrayList<Particle> getParticles() {
        synchronized (particleListLock) {
            return new ArrayList<>(particles);
        }
    }

    public int getNumParticles() {
        return particles.size();
    }

    public boolean isFull() {
        return particles.size() == MAX_LOAD;
    }

    public void addNewParticles(ArrayList<Particle> newParticles) {
        particles.addAll(newParticles);
        numParticles += newParticles.size();
    }

    public void addParticle(Particle particle) {
        synchronized(particleListLock) {
            particles.add(particle);
        }
    }

    public void clearParticles() {
        particles = new ArrayList<>();
    }

    public int getID() {
        return id;
    }

    @Override
    public void run() {
        long lastUpdateTime = System.currentTimeMillis(); // Track the last update time

        while(true) {
            synchronized(particleListLock) {
                long currentTime = System.currentTimeMillis();
                long deltaTime = currentTime - lastUpdateTime; // Calculate elapsed time since last update
                lastUpdateTime = currentTime; // Update lastUpdateTime for the next loop iteration

                for (Particle particle : particles) {
                    particle.update(deltaTime); // Update particles with elapsed time
                }
            }

            try {
                Thread.sleep(10); // Control the update rate
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

}
