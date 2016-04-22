import logging
import time

def worker_thread(job_queue, adaptor, results):
	logging.info("Worker thread started")

	throttling = adaptor.get_throttling()
	job = job_queue.get_job()

	start_time = time.time()
	job.compute()
	results.add_finished_job(job)
	end_time = time.time()

	logging.info("Worker thread job finished")

	sleeping_time = (end_time - start_time) * (1 - throttling)
	logging.warning("Worker thread will sleep for %f seconds..." % sleeping_time)
	time.sleep(sleeping_time)
