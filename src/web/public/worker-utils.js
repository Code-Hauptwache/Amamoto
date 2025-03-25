/**
 * Worker utilities for Amamoto Traffic Simulation
 * This module provides utilities for Web Worker implementation
 */

export const WorkerUtils = {
    /**
     * Split an array into approximately equal-sized chunks
     * @param {Array} array - The array to split
     * @param {number} chunkCount - Number of chunks to create
     * @returns {Array<Array>} Array of chunks
     */
    splitArrayIntoChunks(array, chunkCount) {
        if (!array || array.length === 0) return [];
        if (chunkCount <= 0) return [array];
        
        const chunkSize = Math.ceil(array.length / chunkCount);
        const chunks = [];
        
        for (let i = 0; i < array.length; i += chunkSize) {
            chunks.push(array.slice(i, i + chunkSize));
        }
        
        return chunks;
    },
    
    /**
     * Merge updated vehicle data from worker chunks back into the main array
     * @param {Array} originalVehicles - Original vehicle array
     * @param {Array<Array>} workerResults - Results from different workers
     * @param {Array<Array>} batchIndices - Original indices for each batch
     * @returns {Array} Merged vehicle array
     */
    mergeWorkerResults(originalVehicles, workerResults, batchIndices) {
        // Create a new array with the same length as the original
        const mergedVehicles = [...originalVehicles];
        
        // Update with results from each worker
        for (let i = 0; i < workerResults.length; i++) {
            const batch = workerResults[i];
            const indices = batchIndices[i];
            
            for (let j = 0; j < batch.length; j++) {
                const originalIndex = indices[j];
                mergedVehicles[originalIndex] = batch[j];
            }
        }
        
        return mergedVehicles;
    },
    
    /**
     * Create and initialize simulation workers
     * @param {number} count - Number of workers to create
     * @param {Object} bounds - Simulation bounds
     * @param {boolean} keepInBounds - Keep vehicles within bounds
     * @returns {Promise<Array>} - Promise resolving to array of initialized workers
     */
    createWorkers(count, bounds, keepInBounds) {
        return new Promise((resolve, reject) => {
            const workers = [];
            let initializedCount = 0;
            
            for (let i = 0; i < count; i++) {
                try {
                    const worker = new Worker('./simulation-worker.js', { type: 'module' });
                    workers.push(worker);
                    
                    // Initialize worker
                    worker.postMessage({
                        type: 'init',
                        bounds,
                        keepInBounds
                    });
                    
                    // Listen for initialization response
                    worker.onmessage = (e) => {
                        if (e.data.type === 'initialized') {
                            initializedCount++;
                            
                            if (initializedCount === count) {
                                resolve(workers);
                            }
                        }
                    };
                } catch (err) {
                    reject(`Error creating worker: ${err.message}`);
                }
            }
        });
    },
    
    /**
     * Terminate all simulation workers
     * @param {Array} workers - Array of workers to terminate
     */
    terminateWorkers(workers) {
        if (!workers || !Array.isArray(workers)) return;
        
        for (const worker of workers) {
            if (worker) {
                worker.terminate();
            }
        }
    },
    
    /**
     * Process vehicle updates using Web Workers
     * @param {Array} workers - Array of worker instances
     * @param {Array} vehicles - Array of vehicle data
     * @param {number} dt - Time step for the update
     * @param {boolean} detectCollisions - Whether to perform collision detection
     * @returns {Promise<Array>} Promise resolving to updated vehicle array
     */
    async processVehiclesWithWorkers(workers, vehicles, dt, detectCollisions = true) {
        return new Promise((resolve, reject) => {
            if (!workers || workers.length === 0 || !vehicles || vehicles.length === 0) {
                resolve(vehicles);
                return;
            }
            
            // Get count of active vehicles
            const activeVehicles = vehicles.filter(v => v.active);
            if (activeVehicles.length === 0) {
                resolve(vehicles);
                return;
            }
            
            // Split vehicles into batches for workers
            const vehicleBatches = this.splitArrayIntoChunks(activeVehicles, workers.length);
            
            // Keep track of batch indices in the original array
            const batchIndices = vehicleBatches.map(batch => {
                return batch.map(v => vehicles.indexOf(v));
            });
            
            // Process each batch in a separate worker
            const batchPromises = vehicleBatches.map((batch, index) => {
                return new Promise((resolveBatch) => {
                    const worker = workers[index];
                    
                    const messageHandler = (e) => {
                        const message = e.data;
                        
                        if (message.type === 'vehiclesUpdated') {
                            worker.removeEventListener('message', messageHandler);
                            resolveBatch(message.vehicles);
                        }
                    };
                    
                    worker.addEventListener('message', messageHandler);
                    
                    worker.postMessage({
                        type: 'updateVehicles',
                        vehicles: batch,
                        dt,
                        detectCollisions,
                        batchId: index
                    });
                });
            });
            
            // Wait for all batches to complete
            Promise.all(batchPromises)
                .then(results => {
                    // Merge results back into the original array
                    const mergedVehicles = this.mergeWorkerResults(vehicles, results, batchIndices);
                    resolve(mergedVehicles);
                })
                .catch(error => {
                    reject(`Error in worker processing: ${error}`);
                });
        });
    }
};